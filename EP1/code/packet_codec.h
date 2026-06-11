#ifndef PACKET_CODEC_H
#define PACKET_CODEC_H

#include <stdint.h>
#include <stddef.h>

class PacketCodec {
 public:
  explicit PacketCodec(int fd);

  bool readByte(uint8_t& value);
  bool writeByte(uint8_t value);
  bool readUint16(uint16_t& value);
  bool writeUint16(uint16_t value);
  bool readUint32(uint32_t& value);
  bool writeUint32(uint32_t value);
  bool readUint64(uint64_t& value);
  bool writeUint64(uint64_t value);
  bool readFloat(float& value);
  bool writeFloat(float value);
  bool readDouble(double& value);
  bool writeDouble(double value);

  bool readExact(uint8_t* buf, size_t len);
  bool writeExact(const uint8_t* buf, size_t len);
  bool readVarInt(int32_t& value);
  bool writeVarInt(uint32_t value);
  int sizeVarInt(uint32_t value) const;
  bool readString(char* out, size_t out_len);
  bool writeString(const char* str);
  bool skipString();
  bool skipBytes(size_t len);

  int fd() const { return fd_; }

 private:
  int fd_;
};

#endif
