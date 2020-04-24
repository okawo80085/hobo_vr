import time
import asyncio
import random
from .. import utilz as u


class PoserTemplate:
	"""Poser base class.

	__ini__(*, addr='127.0.0.1', port=6969, sendDelay=1/60, recvDelay=1/1000, **kwargs)

	addr - is the address of the server to connect to, stored in self.addr
	port - is the port of the server to connect to, stored in self.port
	sendDelay - sleep delay for the self.send thread(in seconds)
	recvDelay - sleep delay for the self.recv thread(in seconds)

	self._socket_init() - connects to the server using self.addr and self.port and sends the id message, stores socket reader and writer in self.reader and self.writer, it is not recommended you override this method

	self.main() - main async method which gathers all threads, it is not recommended you override this method

	supplies 3 tracked device pose dicts:
		self.pose - hmd pose, position, orientation, velocity and angular velocity only
		self.poseControllerR - same as hmd pose +controller inputs for controller 1
		self.poseControllerL - same as hmd pose +controller inputs for controller 2
	
	more info on poses/message format:
		https://github.com/okawo80085/hobo_vr/wiki/poser-message-format

	supplies a last message from server buffer:
		self.lastRead - string containing the last message from the server

	supplies threading vars:
		self.coro_list - list of all methods recognized as threads
		self.coro_keepAlive - dict of all registered threads, containing self.coro_keepAlive['threadMetohdName'] = [KeepAliveBool, SleepDelay], the dict is populated at self.main() call

	this base class also has 3 built in threads, it is not recommended you override any of them:
		self.send - sends all pose data to the server
		self.recv - receives messages from the server, the last message is stored in self.lastRead
		self.close - closes the connection to the server and ends all threads that where registered by thread_register

	this base class will assume that every
	child method without '_' as a first character
	in the name is a thread

	every child class also needs to register it's thread
	methods with the thread_register decorator

	example:

		class MyPoser(PoserTemplate):
			def __init__(self, *args, **kwargs):
				super().__init(**kwargs)

			@thread_register(0.5)
			async def example_thread(self):
				while self.coro_keepAlive['example_thread'][0]:
					self.pose['x'] += 0.04

					await asyncio.sleep(self.coro_keepAlive['example_thread'][1])

		poser = MyPoser()

		asyncio.run(poser.main())

	more examples:
		https://github.com/okawo80085/hobo_vr/blob/master/poseTracker.py
		https://github.com/okawo80085/hobo_vr/blob/master/examples/poserTemplate.py

	"""
	def __init__(self, *, addr='127.0.0.1', port=6969, sendDelay=1/60, recvDelay=1/1000, **kwargs):
		self.addr = addr
		self.port = port


		_reserved = ['main']

		self.coro_list = [method_name for method_name in dir(self)
				if callable(getattr(self, method_name)) and method_name[0] != '_' and method_name not in _reserved]

		self.coro_keepAlive = {
			'close':[True, 0.1],
			'send':[True, sendDelay],
			'recv':[True, recvDelay],
		}

		self.pose = {
			# location in meters and orientation in quaternion
			'x':0,	# +(x) is right in meters
			'y':1,	# +(y) is up in meters
			'z':0,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,
		}

		self.poseControllerR = {
			# location in meters and orientation in quaternion
			'x':0.5,	# +(x) is right in meters
			'y':1,	# +(y) is up in meters
			'z':-1,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,

			# inputs
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,	# 0 or 1
			'triggerClick':0,	# 0 or 1
		}

		self.poseControllerL = {
			# location in meters and orientation in quaternion
			'x':0.5,	# +(x) is right in meters
			'y':1.1,	# +(y) is up in meters
			'z':-1,	# -(z) is forward in meters
			'rW':1,	# from 0 to 1
			'rX':0,	# from -1 to 1
			'rY':0,	# from -1 to 1
			'rZ':0,	# from -1 to 1

			# velocity in meters/second
			'velX':0, # +(x) is right in meters/second
			'velY':0, # +(y) is right in meters/second
			'velZ':0, # -(z) is right in meters/second

			# Angular velocity of the pose in axis-angle 
			# representation. The direction is the angle of
			# rotation and the magnitude is the angle around
			# that axis in radians/second.
			'angVelX':0,
			'angVelY':0,
			'angVelZ':0,

			# inputs
			'grip':0,	# 0 or 1
			'system':0,	# 0 or 1
			'menu':0,	# 0 or 1
			'trackpadClick':0,	# 0 or 1
			'triggerValue':0,	# from 0 to 1
			'trackpadX':0,	# from -1 to 1
			'trackpadY':0,	# from -1 to 1
			'trackpadTouch':0,	# 0 or 1
			'triggerClick':0,	# 0 or 1
		}

		self.lastRead = ''

	async def _socket_init(self):
		'''
		connect to the server using self.addr and self.port

		store reader and writer in self.reader and self.writer

		send id message
		more on id messages: https://github.com/okawo80085/hobo_vr/wiki/server-id-messages
		'''
		print (f'connecting to the server at "{self.addr}:{self.port}"...')
		# connect to the server
		self.reader, self.writer = await asyncio.open_connection(self.addr, self.port,												   loop=asyncio.get_event_loop())
		# send poser id message
		self.writer.write(u.convv('poser here'))


	async def send(self):
		'''
		send all poses thread
		'''
		while self.coro_keepAlive['send'][0]:
			try:
				msg = u.convv(' '.join([str(i) for _, i in self.pose.items()] + [str(i) for _, i in self.poseControllerR.items()] + [str(i) for _, i in self.poseControllerL.items()]))

				self.writer.write(msg)

				await asyncio.sleep(self.coro_keepAlive['send'][1])
			except Exception as e:
				print (f'send failed: {e}')
				self.coro_keepAlive['send'][0] = False
				break


	async def recv(self):
		'''
		receive messages thread
		'''
		while self.coro_keepAlive['recv'][0]:
			try:
				data = await u.newRead(self.reader)
				self.lastRead = data

				await asyncio.sleep(self.coro_keepAlive['recv'][1])
			except Exception as e:
				print (f'recv failed: {e}')
				self.coro_keepAlive['recv'][0] = False
				break

	async def close(self):
		'''
		await close thread, press "q" to kill all registered threads
		'''
		try:
			import keyboard

			while self.coro_keepAlive['close'][0]:
				if keyboard.is_pressed('q'):
					self.coro_keepAlive['close'][0] = False
					break

				await asyncio.sleep(self.coro_keepAlive['close'][1])

		except ImportError as e:
			print (f'failed to import keyboard, poser will close in 10 seconds: {e}')
			await asyncio.sleep(10)

		print ('closing...')
		for key in self.coro_keepAlive:
			self.coro_keepAlive[key][0] = False
			print (f'{key} stop sent...')

		await asyncio.sleep(1)

		self.writer.write(u.convv('CLOSE'))
		self.writer.close()

		print ('done')


	async def main(self):
		'''
		main asyncio method, gathers all recognized threads and runs them
		'''
		await self._socket_init()

		await asyncio.gather(
				*[getattr(self, coro_name)() for coro_name in self.coro_list]
			)


def thread_register(sleepDelay, runInDefaultExecutor=False):
	'''
	registers thread for PoserTemplate base class

	sleepDelay - sleep delay in seconds
	runInDefaultExecutor - bool, set True if you want the function to be executed in asyncio's default pool executor
	'''
	def _thread_reg(func):
		def _thread_reg_wrapper(self, *args, **kwargs):
			if func.__name__ not in self.coro_keepAlive and func.__name__ in self.coro_list:
				self.coro_keepAlive[func.__name__] = [True, sleepDelay]

			if runInDefaultExecutor:
				loop = asyncio.get_running_loop()

				return loop.run_in_executor(None, func, self, *args, **kwargs)

			return func(self, *args, **kwargs)

		return _thread_reg_wrapper

	return _thread_reg