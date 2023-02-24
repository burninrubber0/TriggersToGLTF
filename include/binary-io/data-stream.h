#pragma once

#include <QDataStream>

class DataStream : public QDataStream
{
public:
	DataStream(ByteOrder byteOrder = LittleEndian, bool is64Bit = false,
		FloatingPointPrecision precision = SinglePrecision);

	// Get whether the stream reads and writes pointers as 32 or 64 bit
	bool getIs64Bit();

	// Set whether the stream reads and writes pointers as 32 or 64 bit
	void setIs64Bit(bool setting);

	// Reads a pointer from the stream
	template <typename T>
	friend DataStream& operator>>(DataStream& s, T& ptr) requires(std::is_pointer_v<T>)
	{
		if (!s.is64Bit)
			s >> (quint32&)ptr;
		else
			s >> (quint64&)ptr;
		return s;
	}

	// Writes a pointer to the stream
	template <typename T>
	friend DataStream& operator<<(DataStream& s, T& ptr) requires(std::is_pointer_v<T>)
	{
		if (!s.is64Bit)
			s << (quint32&)ptr;
		else
			s << (quint64&)ptr;
		return s;
	}

	// callocs and reads data from the stream using DataStream's operator>>(DataStream&, T&) method
	template <typename T>
	void cAllocAndQtRead(T*& entries, int count)
	{
		seek((qint64)entries); // Seek to the offset in the stream
		entries = (T*)calloc(count, sizeof(T)); // New pointer where the entries will be stored in memory
		assert(entries != nullptr);
		for (int i = 0; i < count; ++i)
			*this >> entries[i];
	}

	// callocs and reads data from the stream using T's read(DataStream&) method
	template <typename T>
	void cAllocAndCustomRead(T*& entries, int count)
	{
		seek((qint64)entries); // Seek to the offset in the stream
		entries = (T*)calloc(count, sizeof(T)); // New pointer where the entries will be stored in memory
		assert(entries != nullptr);
		for (int i = 0; i < count; ++i)
			entries[i].read(*this);
	}

	// Reads a specified number of bytes from this stream into a string
	// In the future, this may be overloaded to read a string of unspecified length
	void readString(QString& string, quint32 length);

	// Writes a string to the stream, optionally limited to a maximum length
	void writeString(QString string, quint64 length = -1);

	// Reads a specified number of bytes from this stream to another stream, writing to a specified
	// offset in the other stream. By default, this offset is the current offset of this stream.
	// This is most useful for storing unknown data and padding to prevent data loss
	void readRawData(DataStream& stream, quint32 length, qint64 position = -1);

	// Get the current offset the device is reading/writing from
	qint64 pos();

	// Opens the device
	void open(QIODeviceBase::OpenMode mode);

	// Seeks to a specified offset in the device
	void seek(qint64 offset);

	// Skips a specified number of bytes in the stream
	void skip(qint64 length);

	// Skips a specified number of bytes in the stream if the buffer is 64 bit
	// Useful for 64 bit-only alignment
	void skipIf64(qint64 length);

	// Closes the device
	void close();

private:
	bool is64Bit;
};
