// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: byte_dict.proto

#include "byte_dict.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
namespace VRProto {
constexpr ByteDict_EntriesEntry_DoNotUse::ByteDict_EntriesEntry_DoNotUse(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized){}
struct ByteDict_EntriesEntry_DoNotUseDefaultTypeInternal {
  constexpr ByteDict_EntriesEntry_DoNotUseDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~ByteDict_EntriesEntry_DoNotUseDefaultTypeInternal() {}
  union {
    ByteDict_EntriesEntry_DoNotUse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ByteDict_EntriesEntry_DoNotUseDefaultTypeInternal _ByteDict_EntriesEntry_DoNotUse_default_instance_;
constexpr ByteDict::ByteDict(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : entries_(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}){}
struct ByteDictDefaultTypeInternal {
  constexpr ByteDictDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~ByteDictDefaultTypeInternal() {}
  union {
    ByteDict _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ByteDictDefaultTypeInternal _ByteDict_default_instance_;
}  // namespace VRProto
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_byte_5fdict_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_byte_5fdict_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_byte_5fdict_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_byte_5fdict_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict_EntriesEntry_DoNotUse, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict_EntriesEntry_DoNotUse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict_EntriesEntry_DoNotUse, key_),
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict_EntriesEntry_DoNotUse, value_),
  0,
  1,
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::VRProto::ByteDict, entries_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 7, sizeof(::VRProto::ByteDict_EntriesEntry_DoNotUse)},
  { 9, -1, sizeof(::VRProto::ByteDict)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::VRProto::_ByteDict_EntriesEntry_DoNotUse_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::VRProto::_ByteDict_default_instance_),
};

const char descriptor_table_protodef_byte_5fdict_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\017byte_dict.proto\022\007VRProto\"k\n\010ByteDict\022/"
  "\n\007entries\030\001 \003(\0132\036.VRProto.ByteDict.Entri"
  "esEntry\032.\n\014EntriesEntry\022\013\n\003key\030\001 \001(\r\022\r\n\005"
  "value\030\002 \001(\002:\0028\001b\006proto3"
  ;
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_byte_5fdict_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_byte_5fdict_2eproto = {
  false, false, 143, descriptor_table_protodef_byte_5fdict_2eproto, "byte_dict.proto", 
  &descriptor_table_byte_5fdict_2eproto_once, nullptr, 0, 2,
  schemas, file_default_instances, TableStruct_byte_5fdict_2eproto::offsets,
  file_level_metadata_byte_5fdict_2eproto, file_level_enum_descriptors_byte_5fdict_2eproto, file_level_service_descriptors_byte_5fdict_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK ::PROTOBUF_NAMESPACE_ID::Metadata
descriptor_table_byte_5fdict_2eproto_metadata_getter(int index) {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_byte_5fdict_2eproto);
  return descriptor_table_byte_5fdict_2eproto.file_level_metadata[index];
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_byte_5fdict_2eproto(&descriptor_table_byte_5fdict_2eproto);
namespace VRProto {

// ===================================================================

ByteDict_EntriesEntry_DoNotUse::ByteDict_EntriesEntry_DoNotUse() {}
ByteDict_EntriesEntry_DoNotUse::ByteDict_EntriesEntry_DoNotUse(::PROTOBUF_NAMESPACE_ID::Arena* arena)
    : SuperType(arena) {}
void ByteDict_EntriesEntry_DoNotUse::MergeFrom(const ByteDict_EntriesEntry_DoNotUse& other) {
  MergeFromInternal(other);
}
::PROTOBUF_NAMESPACE_ID::Metadata ByteDict_EntriesEntry_DoNotUse::GetMetadata() const {
  return GetMetadataStatic();
}
void ByteDict_EntriesEntry_DoNotUse::MergeFrom(
    const ::PROTOBUF_NAMESPACE_ID::Message& other) {
  ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom(other);
}


// ===================================================================

class ByteDict::_Internal {
 public:
};

ByteDict::ByteDict(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena),
  entries_(arena) {
  SharedCtor();
  RegisterArenaDtor(arena);
  // @@protoc_insertion_point(arena_constructor:VRProto.ByteDict)
}
ByteDict::ByteDict(const ByteDict& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  entries_.MergeFrom(from.entries_);
  // @@protoc_insertion_point(copy_constructor:VRProto.ByteDict)
}

void ByteDict::SharedCtor() {
}

ByteDict::~ByteDict() {
  // @@protoc_insertion_point(destructor:VRProto.ByteDict)
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

void ByteDict::SharedDtor() {
  GOOGLE_DCHECK(GetArena() == nullptr);
}

void ByteDict::ArenaDtor(void* object) {
  ByteDict* _this = reinterpret_cast< ByteDict* >(object);
  (void)_this;
}
void ByteDict::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void ByteDict::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void ByteDict::Clear() {
// @@protoc_insertion_point(message_clear_start:VRProto.ByteDict)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  entries_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ByteDict::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // map<uint32, float> entries = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 10)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(&entries_, ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<10>(ptr));
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* ByteDict::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:VRProto.ByteDict)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // map<uint32, float> entries = 1;
  if (!this->_internal_entries().empty()) {
    typedef ::PROTOBUF_NAMESPACE_ID::Map< ::PROTOBUF_NAMESPACE_ID::uint32, float >::const_pointer
        ConstPtr;
    typedef ::PROTOBUF_NAMESPACE_ID::internal::SortItem< ::PROTOBUF_NAMESPACE_ID::uint32, ConstPtr > SortItem;
    typedef ::PROTOBUF_NAMESPACE_ID::internal::CompareByFirstField<SortItem> Less;

    if (stream->IsSerializationDeterministic() &&
        this->_internal_entries().size() > 1) {
      ::std::unique_ptr<SortItem[]> items(
          new SortItem[this->_internal_entries().size()]);
      typedef ::PROTOBUF_NAMESPACE_ID::Map< ::PROTOBUF_NAMESPACE_ID::uint32, float >::size_type size_type;
      size_type n = 0;
      for (::PROTOBUF_NAMESPACE_ID::Map< ::PROTOBUF_NAMESPACE_ID::uint32, float >::const_iterator
          it = this->_internal_entries().begin();
          it != this->_internal_entries().end(); ++it, ++n) {
        items[static_cast<ptrdiff_t>(n)] = SortItem(&*it);
      }
      ::std::sort(&items[0], &items[static_cast<ptrdiff_t>(n)], Less());
      for (size_type i = 0; i < n; i++) {
        target = ByteDict_EntriesEntry_DoNotUse::Funcs::InternalSerialize(1, items[static_cast<ptrdiff_t>(i)].second->first, items[static_cast<ptrdiff_t>(i)].second->second, target, stream);
      }
    } else {
      for (::PROTOBUF_NAMESPACE_ID::Map< ::PROTOBUF_NAMESPACE_ID::uint32, float >::const_iterator
          it = this->_internal_entries().begin();
          it != this->_internal_entries().end(); ++it) {
        target = ByteDict_EntriesEntry_DoNotUse::Funcs::InternalSerialize(1, it->first, it->second, target, stream);
      }
    }
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:VRProto.ByteDict)
  return target;
}

size_t ByteDict::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:VRProto.ByteDict)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // map<uint32, float> entries = 1;
  total_size += 1 *
      ::PROTOBUF_NAMESPACE_ID::internal::FromIntSize(this->_internal_entries_size());
  for (::PROTOBUF_NAMESPACE_ID::Map< ::PROTOBUF_NAMESPACE_ID::uint32, float >::const_iterator
      it = this->_internal_entries().begin();
      it != this->_internal_entries().end(); ++it) {
    total_size += ByteDict_EntriesEntry_DoNotUse::Funcs::ByteSizeLong(it->first, it->second);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void ByteDict::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:VRProto.ByteDict)
  GOOGLE_DCHECK_NE(&from, this);
  const ByteDict* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<ByteDict>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:VRProto.ByteDict)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:VRProto.ByteDict)
    MergeFrom(*source);
  }
}

void ByteDict::MergeFrom(const ByteDict& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:VRProto.ByteDict)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  entries_.MergeFrom(from.entries_);
}

void ByteDict::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:VRProto.ByteDict)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void ByteDict::CopyFrom(const ByteDict& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:VRProto.ByteDict)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ByteDict::IsInitialized() const {
  return true;
}

void ByteDict::InternalSwap(ByteDict* other) {
  using std::swap;
  _internal_metadata_.Swap<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(&other->_internal_metadata_);
  entries_.Swap(&other->entries_);
}

::PROTOBUF_NAMESPACE_ID::Metadata ByteDict::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace VRProto
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::VRProto::ByteDict_EntriesEntry_DoNotUse* Arena::CreateMaybeMessage< ::VRProto::ByteDict_EntriesEntry_DoNotUse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::VRProto::ByteDict_EntriesEntry_DoNotUse >(arena);
}
template<> PROTOBUF_NOINLINE ::VRProto::ByteDict* Arena::CreateMaybeMessage< ::VRProto::ByteDict >(Arena* arena) {
  return Arena::CreateMessageInternal< ::VRProto::ByteDict >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
