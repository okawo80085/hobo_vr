import asyncio
import time
import cv2
import numpy as np
import math as m

from pykalman import KalmanFilter


def convv(sss):
	if len(sss) < 1:
		return ''.encode('utf-8')

	if sss[-1] != '\n':
		return (sss + '\n').encode('utf-8')

	return sss.encode('utf-8')

async def newRead(reader):
	data = []
	temp = None
	while temp != '\n' and temp != '':
		temp = await reader.read(1)
		temp = temp.decode('utf-8')
		data.append(temp)

	return ''.join(data)

def rotateZ(points, angle):
	sin = np.sin(angle)
	cos = np.cos(angle)

	for key, p in points.items():
		x = p['x']*cos - p['y']*sin
		y = p['y']*cos + p['x']*sin

		points[key]['x'] = x
		points[key]['y'] = y

def rotateX(points, angle):
	sin = np.sin(angle)
	cos = np.cos(angle)

	for key, p in points.items():
		y = p['y']*cos - p['z']*sin
		z = p['z']*cos + p['y']*sin

		points[key]['y'] = y
		points[key]['z'] = z

def rotateY(points, angle):
	sin = np.sin(angle)
	cos = np.cos(angle)

	for key, p in points.items():
		x = p['x']*cos - p['z']*sin
		z = p['z']*cos + p['x']*sin

		points[key]['x'] = x
		points[key]['z'] = z

def hasCharsInString(str1, str2):
	for i in str1:
		if i in str2:
			return True

	return False

def getOnlyNums(text):
	if not hasCharsInString(text, 'aqzwsxedcrfvtgbyhnujmikolp[]{}*<>,') and len(text) != 0:
		out = []
		try:
			for i in text.strip('\n').strip('\r').split('\t'):
				out.append(float(i))

			if len(out) < 4:
				raise Exception('to few numbers found')

		except:
			# print (text, len(text))
			out = [0, 0, 0, 0]

		return out
	# print (text, len(text))

	return [0, 0, 0, 0]

def hasNanInPose(pose):
	for key in ['x', 'y', 'z']:
		if np.isnan(pose[key]) or np.isinf(pose[key]):
			return True

	return False


class Tracker:
	def __init__(self, cam_index=4, focal_length_px=490, ball_size_cm=2):
		self.vs = cv2.VideoCapture(cam_index)

		self.camera_focal_length = focal_length_px
		self.BALL_RADIUS_CM = ball_size_cm

		time.sleep(2)

		self.can_track, frame = self.vs.read()

		if not self.can_track:
			self.vs.release()

		self.frame_height, self.frame_width, _ = frame.shape if self.can_track else (0, 0, 0)


		self.markerMasks = {
		'blue':{
			'h':(97, 10),
			's':(256, 55),
			'v':(250, 32)
				},
		'red':{
			'h':(18, 24),
			's':(210, 65),
			'v':(200, 62)
			},
		'green':{
			'h':(69, 20),
			's':(140, 55),
			'v':(230, 50)
			}
		}

		self.poses = {}
		self.blobs = {}

		self.kalmanTrainSize = 15
		self.kalmanFilterz = {}
		self.kalmanTrainBatch = {}
		self.kalmanWasInited = {}
		self.kalmanStateUpdate = {}

		for i in self.markerMasks:
			self.poses[i] = {'x':0, 'y':0, 'z':0}
			self.blobs[i] = None

			self.kalmanFilterz[i] = None
			self.kalmanStateUpdate[i] = None
			self.kalmanTrainBatch[i] = []
			self.kalmanWasInited[i] = False

	def _reinit(self):
		self.can_track, frame = self.vs.read()

		if not self.can_track:
			self.vs.release()

		self.frame_height, self.frame_width, _ = frame.shape if self.can_track else (0, 0, 0)

		self.poses = {}
		self.blobs = {}

		self.kalmanFilterz = {}
		self.kalmanTrainBatch = {}
		self.kalmanWasInited = {}
		self.kalmanStateUpdate = {}

		for i in self.markerMasks:
			self.poses[i] = {'x':0, 'y':0, 'z':0}
			self.blobs[i] = None

			self.kalmanFilterz[i] = None
			self.kalmanStateUpdate[i] = None
			self.kalmanTrainBatch[i] = []
			self.kalmanWasInited[i] = False

	def _initKalman(self, key):
		self.kalmanTrainBatch[key] = np.array(self.kalmanTrainBatch[key])

		initState = [*self.kalmanTrainBatch[key][0]]

		transition_matrix = [[1, 0, 0],
							 [0, 1, 0],
							 [0, 0, 1]]

		observation_matrix = [[1, 0, 0],
							  [0, 1, 0],
							  [0, 0, 1]]

		self.kalmanFilterz[key] = KalmanFilter(transition_matrices = transition_matrix,
										 observation_matrices = observation_matrix,
										 initial_state_mean = initState)

		print (f'initializing Kalman filter for mask "{key}"...')

		tStart = time.time()
		self.kalmanFilterz[key] = self.kalmanFilterz[key].em(self.kalmanTrainBatch[key], n_iter=5)
		print (f'took: {time.time() - tStart}')

		fMeans, fCovars = self.kalmanFilterz[key].filter(self.kalmanTrainBatch[key])
		self.kalmanStateUpdate[key] = {'x_now':fMeans[-1, :], 'p_now':fCovars[-1, :]}

		self.kalmanWasInited[key] = True

	def _applyKalman(self, key):
		obz = np.array([self.poses[key]['x'], self.poses[key]['y'], self.poses[key]['z']])
		if self.kalmanFilterz[key] is not None:
			self.kalmanStateUpdate[key]['x_now'], self.kalmanStateUpdate[key]['p_now'] = self.kalmanFilterz[key].filter_update(filtered_state_mean = self.kalmanStateUpdate[key]['x_now'],
																															 filtered_state_covariance = self.kalmanStateUpdate[key]['p_now'],
																															 observation = obz)

			self.poses[key]['x'], self.poses[key]['y'], self.poses[key]['z'] = self.kalmanStateUpdate[key]['x_now']

	def getFrame(self):
		self.can_track, frame = self.vs.read()

		for key, maskRange in self.markerMasks.items():
			hc, hr = maskRange['h']

			sc, sr = maskRange['s']

			vc, vr = maskRange['v']

			colorHigh = [hc+hr, sc+sr, vc+vr]
			colorLow = [hc-hr, sc-sr, vc-vr]

			if self.can_track:
				blurred = cv2.GaussianBlur(frame, (3, 3), 0)

				hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

				mask = cv2.inRange(hsv, tuple(colorLow), tuple(colorHigh))

				cnts, hr = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
					cv2.CHAIN_APPROX_SIMPLE)

				if len(cnts) > 0:
					c = max(cnts, key=cv2.contourArea)
					self.blobs[key] = c if len(c) >= 5 and cv2.contourArea(c) > 10 else None

	def solvePose(self):
		for key, blob in self.blobs.items():
			if blob is not None:
				elip = cv2.fitEllipse(blob)

				x, y = elip[0]
				w, h = elip[1]

				f_px = self.camera_focal_length
				X_px = x
				Y_px = y
				A_px = w/2

				X_px -= self.frame_width/2
				Y_px = self.frame_height/2 - Y_px

				L_px = np.sqrt(X_px**2 + Y_px**2)

				k = L_px / f_px

				j = (L_px + A_px) / f_px

				l = (j - k) / (1+j*k)

				D_cm = self.BALL_RADIUS_CM * np.sqrt(1+l**2)/l

				fl = f_px/L_px

				Z_cm = D_cm * fl/np.sqrt(1+fl**2)

				L_cm = Z_cm*k

				X_cm = L_cm * X_px/L_px
				Y_cm = L_cm * Y_px/L_px

				self.poses[key]['x'] = X_cm/100
				self.poses[key]['y'] = Y_cm/100
				self.poses[key]['z'] = Z_cm/100

				if len(self.kalmanTrainBatch[key]) < self.kalmanTrainSize:
					self.kalmanTrainBatch[key].append([self.poses[key]['x'], self.poses[key]['y'], self.poses[key]['z']])

				elif len(self.kalmanTrainBatch[key]) >= self.kalmanTrainSize and not self.kalmanWasInited[key]:
					self._initKalman(key)

				else:
					if not hasNanInPose(self.poses[key]):
						self._applyKalman(key)


	def close(self):
		if self.can_track:
			self.vs.release()

		cv2.destroyAllWindows()