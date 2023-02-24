#include <binary-io/data-stream.h>

#include <QIODevice>

DataStream::DataStream(ByteOrder byteOrder, bool is64Bit, FloatingPointPrecision precision)
	: is64Bit(is64Bit)
{
	// Set endianness (default is big endian)
	setByteOrder(byteOrder);

	// Set single precision FP (default is double)
	setFloatingPointPrecision(precision);
}

// Get whether the stream reads and writes pointers as 32 or 64 bit
bool DataStream::getIs64Bit()
{
	return is64Bit;
}

// Set whether the stream reads and writes pointers as 32 or 64 bit
void DataStream::setIs64Bit(bool setting)
{
	is64Bit = setting;
}

// Reads a specified number of bytes from this stream into a string
// In the future, this may be overloaded to read a string of unspecified length
void DataStream::readString(QString& string, quint32 length)
{
	string = QString::fromLatin1((device()->read(length)));
}

// Writes a string to the stream, optionally limited to a maximum length
void DataStream::writeString(QString string, quint64 length)
{
	device()->write(string.toLatin1(), length == -1 ? string.size() : length);
}

// Reads a specified number of bytes from this stream to another stream, writing to a specified
// offset in the other stream. By default, this offset is the current offset of this stream.
// This is most useful for storing unknown data and padding to prevent data loss
// TODO: Come up with a more accurate name for this
void DataStream::readRawData(DataStream& stream, quint32 length, qint64 position)
{
	position != -1 ? stream.seek(position) : stream.seek(pos());
	stream.device()->write(device()->read(length));
}

// Get the current offset the device is reading/writing from
qint64 DataStream::pos()
{
	return device()->pos();
}

// Opens the device
void DataStream::open(QIODeviceBase::OpenMode mode)
{
	device()->open(mode);
}

// Seeks to a specified offset in the device
void DataStream::seek(qint64 offset)
{
	device()->seek(offset);
}

// Skips a specified number of bytes in the stream
void DataStream::skip(qint64 length)
{
	device()->skip(length);
}

// Skips a specified number of bytes in the stream if the buffer is 64 bit
// Useful for 64 bit-only alignment
void DataStream::skipIf64(qint64 length)
{
	if (is64Bit)
		device()->skip(length);
}

// Closes the device
void DataStream::close()
{
	device()->close();
}
