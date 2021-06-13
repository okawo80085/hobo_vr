import vrsubscriber
import zmq
from fbs import Controller, Pose, Vec3, Vec4, ByteDict, ByteDictEntry
import flatbuffers
import time

def test_talk_to_self():
	builder = flatbuffers.Builder(0)

	Pose.PoseStart(builder)
	pos = Vec3.CreateVec3(builder, 1, 2, 3)
	Pose.PoseAddPos(builder, pos)
	quat = Vec4.CreateVec4(builder, 2, 3, 4, 1)
	Pose.PoseAddQuat(builder, quat)
	pose = Pose.PoseEnd(builder)

	ByteDictEntry.ByteDictEntryStart(builder)
	ByteDictEntry.ByteDictEntryAddKey(builder, 1)
	ByteDictEntry.ByteDictEntryAddValue(builder, 3.5)
	bde = ByteDictEntry.ByteDictEntryEnd(builder)

	ByteDict.ByteDictStartEntriesVector(builder, 1)
	builder.PrependSOffsetTRelative(bde)
	entries = builder.EndVector()

	ByteDict.ByteDictStart(builder)
	
	ByteDict.ByteDictAddEntries(builder, entries)
	bd = ByteDict.ByteDictEnd(builder)

	Controller.ControllerStart(builder)
	Controller.ControllerAddPose(builder, pose)
	Controller.ControllerAddButtons(builder, bd)
	controller = Controller.ControllerEnd(builder)

	builder.Finish(controller)

	buf = builder.Output()

	con = Controller.Controller.GetRootAsController(buf, 0)

	'''print(con.Buttons())
	print(con.Buttons().EntriesLength())
	print(con.Buttons().Entries(0))
	print(con.Buttons().Entries(1))
	print(con.Buttons().Entries(0).Key())
	print(con.Buttons().Entries(0).Value())
	print(con.Pose())
	print(con.Pose().Quat())
	print(con.Pose().Pos())
	
	print(con.Pose().Quat().W())
	print(con.Pose().Quat().X())
	print(con.Pose().Quat().Y())
	print(con.Pose().Quat().Z())'''

	print(con.Pose().Pos().X())
	'''print(con.Pose().Pos().Y())
	print(con.Pose().Pos().Z())'''


	print("test start")
	vrs = vrsubscriber.vrserver()
	print("got vrs!")

	vrs.start()
	print("started vrs")
	time.sleep(.2)

	context = zmq.Context()
	socket = context.socket(zmq.REQ)
	socket.connect("tcp://127.0.0.1:6969")
	time.sleep(.2)  # wait for req to fully bind
	socket.send_multipart(["DriverSender".encode(), buf])
	msg = socket.recv()
	print(f"Got message: {msg}")
	#socket.send("DriverSender".encode(), zmq.SNDMORE)
	#socket.send(buf)
	print("Sent controller flatbuffer")
	time.sleep(.3)

	#vrs.send("hi")
	print("said hi!")

if __name__=='__main__':
	test_talk_to_self()