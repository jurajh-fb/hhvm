/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#pragma once

namespace HPHP::serialize {

/**
 *                         FB Serialize
 *                         ============
 *
 * === Format ===
 *
 * A value is serialized as a string <c> <data> where c is a byte code,
 * code being one of:
 *
 *  1 (STOP): no data
 *      Marks the end of a STRUCT.
 *
 *  2 (BYTE):  data is 1 byte, signed int8
 *  4 (INT16): data is 2 bytes, network order signed int16
 *  6 (INT32): data is 4 bytes, network order signed int32
 *  8 (INT64): data is 8 bytes, network order signed int64
 *      All of these represent an int64 value.
 *
 *  9 (STRING): followed by 4 byte n (network order unsigned int32), followed
 *      by n characters. All of these represent a string value.
 *
 *  10 (STRUCT): followed by serialized key/value pairs until STOP
 *      is seen.  Represents a map with arbitrary int64 or string keys.
 *
 *  14 (NULL): no data, null value
 *
 *  15 (VARCHAR): followed by 1 byte n (unsigned int8), followed by n characters
 *      All of these represent a string value.
 *
 *  16 (DOUBLE): data is 8 bytes, double value
 *
 *  17 (BOOLEAN): data is 1 byte
 *
 *  18 (VECTOR): followed by a list of values until STOP is seen. Represents a
 *  vector.
 *
 *  19 (LIST): like VECTOR but whose serialization will write the
 *                   FB_SERIALIZE_LIST code. This is unlike the
 *                   VECTOR serialization which writes
 *                   FB_SERIALIZE_STRUCT (instead of FB_SERIALIZE_VECTOR)
 *                   for reasons beyond me. The length of the list is first
 *                   written followed by each element in the list.
 *
 *  20 (SET): followed set size and then each element of the set. On-the-wire
 *            format is basically the same as LIST, except it uses
 *            FB_SERIALIZE_SET tag. Set elements must be strings or integers.
 *            Represents Hack keyset.
 */

enum class Type {
  NULLT,
  BOOL,
  DOUBLE,
  INT64,
  VECTOR,
  MAP,
  STRING,
  OBJECT,
  LIST,
  SET,
};

struct FBSerializeBase {
  enum Code {
    FB_SERIALIZE_STOP    = 1,
    FB_SERIALIZE_BYTE    = 2,
    FB_SERIALIZE_I16     = 4,
    FB_SERIALIZE_I32     = 6,
    FB_SERIALIZE_I64     = 8,
    FB_SERIALIZE_STRING  = 9,
    FB_SERIALIZE_STRUCT  = 10,
    FB_SERIALIZE_NULL    = 14,
    FB_SERIALIZE_VARCHAR = 15,
    FB_SERIALIZE_DOUBLE  = 16,
    FB_SERIALIZE_BOOLEAN = 17,
    FB_SERIALIZE_VECTOR  = 18,
    FB_SERIALIZE_LIST    = 19,
    FB_SERIALIZE_SET     = 20,
  };

  static const size_t CODE_SIZE = 1;
  static const size_t BOOLEAN_SIZE = 1;
  static const size_t INT8_SIZE = 1;
  static const size_t INT16_SIZE = 2;
  static const size_t INT32_SIZE = 4;
  static const size_t INT64_SIZE = 8;
  static const size_t DOUBLE_SIZE = 8;
};

template <class V>
struct FBSerializer : private FBSerializeBase {
  template <typename Variant>
  static size_t serializedSize(const Variant& thing);
  template <typename Variant>
  static void serialize(const Variant& thing, char* out);
 private:
  char* out_;
  explicit FBSerializer(char* out);

  void write(const char* src, size_t size);
  template <typename Variant>
  void doSerialize(const Variant& thing);
  void writeCode(Code code);
  void serializeBoolean(bool val);
  void serializeInt64(int64_t val);
  void serializeDouble(double val);
  template <typename String>
  void serializeString(const String& str);
  template <typename Map>
  void serializeMap(const Map& map, size_t depth);
  template <typename Vector>
  void serializeVector(const Vector& vec, size_t depth);
  template <typename Vector>
  void serializeList(const Vector& vec, size_t depth);
  template <typename Set>
  void serializeSet(const Set& set, size_t depth);
  template <typename Variant>
  void serializeThing(const Variant& thing, size_t depth);

  static size_t serializedSizeInt64(int64_t v);
  template <typename String>
  static size_t serializedSizeString(const String& v);
  template <typename Map>
  static size_t serializedSizeMap(const Map& v, size_t depth);
  template <typename Vector>
  static size_t serializedSizeVector(const Vector& v, size_t depth);
  template <typename Vector>
  static size_t serializedSizeList(const Vector& v, size_t depth);
  template <typename Set>
  static size_t serializedSizeSet(const Set& v, size_t depth);
  template <typename Variant>
  static size_t serializedSizeThing(const Variant& v, size_t depth);
};

template <class V>
struct FBUnserializer : private FBSerializeBase {
  static typename V::VariantType unserialize(folly::StringPiece serialized);

  explicit FBUnserializer(folly::StringPiece serialized);

  bool unserializeBoolean();
  int64_t unserializeInt64();
  double unserializeDouble();
  typename V::StringType unserializeString();
  folly::StringPiece unserializeStringPiece();
  typename V::MapType unserializeMap(size_t depth);
  typename V::VectorType unserializeVector(size_t depth);
  typename V::VectorType unserializeList(size_t depth);
  typename V::SetType unserializeSet(size_t depth);
  // Skip over next bool/int/double/str/struct value
  void skipNext();
  // read the next map but don't unserialze it (for lazy or delay
  // unserialization)
  folly::StringPiece getSerializedMap();
  typename V::VariantType unserializeThing(size_t depth);

  void advance(size_t delta);
  Code nextCode() const;
  bool done() const {
    return p_ == end_;
  }
  // Current position in the serialized buffer
  const char* position() const {
    return p_;
  }
 private:
  void need(size_t n) const;

  const char* p_;
  const char* end_;
};

}

#include "FBSerialize-inl.h"
