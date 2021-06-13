# automatically generated by the FlatBuffers compiler, do not modify

# namespace: fbs

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class ByteDictEntry(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAsByteDictEntry(cls, buf, offset):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ByteDictEntry()
        x.Init(buf, n + offset)
        return x

    # ByteDictEntry
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ByteDictEntry
    def Key(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int8Flags, o + self._tab.Pos)
        return 0

    # ByteDictEntry
    def Value(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Float32Flags, o + self._tab.Pos)
        return 0.0

def ByteDictEntryStart(builder): builder.StartObject(2)
def ByteDictEntryAddKey(builder, key): builder.PrependInt8Slot(0, key, 0)
def ByteDictEntryAddValue(builder, value): builder.PrependFloat32Slot(1, value, 0.0)
def ByteDictEntryEnd(builder): return builder.EndObject()