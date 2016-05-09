
#ifndef __CCC_ZERO_COPY_STREAM_IMPL_H__
#define __CCC_ZERO_COPY_STREAM_IMPL_H__

#include <string>
#include <iosfwd>
#include <zero_copy_stream.h>


// A generic traditional input stream interface.
// 
// Lots of traditional input streams (e.g. file descriptors, C stdio
// streams, and C++ iostreams) expose an interface where every read
// involves copying bytes into a buffer. If you want to take such an 
// interface and make a ZeroCopyInputStream based on it, simply implement
// CopyingInputStream and then use CopyingInputStreamAdaptor
// 
// CopyingInputStream implementations should avoid buffering if possible.
// CopyingInputStreamAdaptor does its own buffering and will read data
// in large blocks.
class CopyingInputStream
{
public:
	virtual ~CopyingInputStream();

	// Reads up to "size" bytes into the given buffer. Returns the number of
	// bytes read. Read() waits until at least one is available, or 
	// returns zero if no bytes will ever become available (EOF)m, or -1 if a 
	// permanent read erroroccurred.
	virtual int Read(void* buffer, int size) = 0;

	// Skips the next "count" bytes of input. Returns the number of bytes
	// actually sipped. This will always be 
};

class FileInputStream : public ZeroCopyInputStream
{
public:
	// Creates a stream that reads from the given Unix file descriptor.
	// If a block size is given, it specifies the number of bytes that
	// should be read and returned with each call to Next(). Otherwise,
	// a reasonable default is used.
	explicit FileInputStream(int file_descriptor, int block_size = -1);
	~FileInputStream());

	// Flushes any buffers and closes the underlying file. Returns false if 
	// an error occurs during the process; use GetErrno() to examine the error.
	// Even if an error occurs, the file descriptor is closed when this returns.
	bool Close();

	// By default, the file descriptor is not closed when the stream is 
	// destroyed. Call SetCloseOnDelete(true) to change that. WARNING:
	// This leaves no way for the caller to detect if close() fails. If
	// detecting close() errors is important to you, you should arrange
	// to close the descriptor yourself.
	void SetCloseOnDelete(bool value) { copying_input_.SetCloseOnDelete(value); }


	// If an I/O error has occurred on this file descriptor, this is the 
	// errno from that error. Otherwise, this is zero. Once an error
	// occurs, the steram is broken and all subsequent operations will
	// fail.
	int GetErrno() { return copying_input_.GetErrno(); }

	// implements ZeroCopyInputStream ------------
	bool Next(const void** data, int* size);
	void BackUp(int count);
	bool Skip(int count);
	long ByteCount() const;

private:
	class CopyingFileInputStream : public CopyingInputStream
	{
	public:
		CopyingFileInputStream(int file_descriptor);
		~CopyingFileInputStream();

		bool Close();
		void SetCloseOnDelete(bool value) { close_on_delete_ =  value; }
		int GetErrno() { return errno_; }

		// implements CopyingInputStream ----------
		int Read(void* buffer, int size);
		int Skip(int count);

	private:
		// The file descriptor
		const int file_;
		bool close_on_delete_;
		bool is_closed_;

		// The errno of the I/O error, if one has occurred. Otherwise, zero.
		int errno_;

		// Did we try to seek once and fail? If so, we assume this file descriptor
		// doesn't support seeking and won't try again.
		bool previous_seek_failed_;

		// DISALLOW EVIL CONSTRUCTORS
		CopyingFileInputStream(const CopyingFileInputStream& );
		void operator= (const CopyingFileInputStream& );
	};

	CopyingFileInputStream copying_input_;
	CopyingInputStreamAdaptor impl_;

	FileInputStream(const FileInputStream& );
	void operator= (const FileInputStream& );
};

// A ZeroCopyOutputStream which writes to a file descriptor.
// 
// FileOutputStream is preferred over using an ofstream with
// OstreamOutputStream. The latter will instroduce an extra layer of buffering.
// harming performance. Also, it's conceivable that FileOutputStream could
// someday be enhanced to use zero-copy file descriptor on OSs which 
// support them.
class FileOutputStream : public ZeroCopyOutputStream
{
public:
	// Creates a steram that writes to the given Unix file descriptor.
	// If a block_size is given, it specifies the size of the buffers
	// that should be returned by Next(). Otherwise, a reasonable default
	// is used.
	explicit FileOutputStream(int file_descriptor, int block_size = -1);
	~FileOutputStream();

	// Flushes any buffers and closes the underlying file. Returns false if 
	// an error occurs during the process; use GetErrno() to examine the error.
	// Even if an error occurs, the file descriptor is closed when this returns.
	bool Close();

	// Flushes FileOutputStream's buffers but does not close the 
	// underlying file. No special measures are taken to ensure that
	// underlying operating system file object is synchronized to disk.
	bool Flush();

	// By default, the file descriptor is not closed when the stream is
	// destroyed. Call SetCloseOnDelete(true) to change that. WARNING:
	// This leaves no way for the caller to detect if close() fails. If 
	// detecting close() errors is important to you, you should arrange
	// to close the descriptor yourself.
	void SetCloseOnDelete(bool value) { copying_output_.SetCloseOnDelete(value); }

	// If an I/O error has occurred on this file descriptor, this is the 
	// errno from that error. Otherwise, this is zero. Once an error
	// occurs, the steram is broken and all subsequent operations will 
	// fail.
	int GetErrno() { return copying_output_.GetErrno(); }

	// implements ZeroCopyOutputStream ----------
	bool Next(void** data, int* size);
	void BackUp(int count);
	long ByteCount() const;

private:
	class CopyingFileOutputStream : public CopyingOutputStream
	{
	public:
		CopyingFileOutputStream(int file_descriptor);
		~CopyingFileOutputStream();

		bool Close();
		void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
		int GetErrno() { return errno_; }

		// implements CopyingOutputStream ------
		bool Write(const void* buffer, int size);

	private:
		// The file descriptor.
		const int file_;
		bool close_on_delete_;
		bool is_closed_;

		// The errno of the I/O error, if one has occurred. Otherwise, zero.
		int errno_;

		// DISALLOW EVIL CONSTRUCTORS
		CopyingFileOutputStream(const CopyingFileOutputStream& );
		void operator= (const CopyingFileOutputStream& );
	};

	CopyingFileOutputStream copying_output_;
	CopyingOutputStreamAdaptor impl_;

	// DISALLOW EVIL CONSTRUCTORS
	FileOutputStream(const FileOutputStream& );
	void operator= (const FileOutputStream& );

};

#endif /// __CCC_ZERO_COPY_STREAM_IMPL_H__