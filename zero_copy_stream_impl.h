
#ifndef __CCC_ZERO_COPY_STREAM_IMPL_H__
#define __CCC_ZERO_COPY_STREAM_IMPL_H__

#include <string>
#include <iosfwd>
#include <zero_copy_stream.h>
#include <common.h>

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
	// actually sipped. This will always be exactly equal to "count" unless
	// EOF was reached or a permanent read error occurred.
	// 
	//  Ethe default implementation just repeatedly calls Read() into a scratch
	//  buffer.
	virtual int Skip(int count); 
};

// A ZeroCopyInputStream which reads from a CopyingInputStream. This is 
// useful for implementing ZeroCopyInputStreams that read from traditional
// streams.  Note that this class is not really zero-copy.
// 
// If you want to read from file descriptors or C++ istreams, this is
// already implemented for you:  use FileInputStream or IstreamInputStream
// respectively
class CopyingInputStreamAdaptor : public ZeroCopyInputStream {
public:
	// Creates a stream that reads from the given CopyingInputStream.
	// If a block_size is given, it specifies the number of bytes that
	// should be read and returned with each call to Next(). Otherwise,
	// a reasonable default is used. The caller retains ownership of 
	// copying_stream unless SetOwnsCopyingStream(true) is called.
	explicit CopyingInputStreamAdaptor(CopyingInputStream* copying_stream,
									   int block_size = -1);
	~CopyingInputStreamAdaptor();

	// Call SetownsCopyingStream(true) to tell the CopyingInputStreamAdaptor to
	// delete the underlying CopyingInputStream when it is destroyed.
	void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

	// implements ZeroCopyInputStream -----------------
	bool Next(const void** data, int* size);
	void BackUp(int count);
	bool Skip(int count);
	long ByteCount() const;

private:
	// Insures that buffer_ is not NULL.
	void AllocateBufferIfNeeded();
	// Frees the buffer and resets buffer_used_.
	void FreeBuffer();

	// The underlying copying stream.
	CopyingInputStream* copying_stream_;
	bool owns_copying_stream_;

	// True if we have seen a permenant error from the underlying stream.
	bool failed_;

	// The current position of copying_stream_, relative to the point where
	// we started reading.
	long position_;

	// Data is read into this buffer. It may be NULL if no buffer is currently
	// in use. Otherwise, it points to an array of size buffer_size_.
	scoped_array<uint8_t> buffer_;
	const int buffer_size_;

	// Number of valid bytes currently in the buffer (i.e. the size last
	// returned by Next()). 0 <= buffer_used_ <= buffer_size_.
	int buffer_used_;

	// Number of bytes in the buffer which were backed up over by a call to 
	// BackUp(). These need to be returned again.
	// 0 <= backup_bytes_ <= buffer_used_
	int backup_bytes_;

	// DISALLOW EVIL CONSTRUCTORS
	CopyingInputStreamAdaptor(const CopyingInputStreamAdaptor&);
	void operator= (const CopyingInputStreamAdaptor&);
};

// A generic traditional output stream interface.
// Lots of traditional output streams (e.g. file descriptors, C stdio
// steram, and C++ iosterams) expose an interface where every write
// involves copying bytes from a buffer. If you want to take such an 
// interface and make a ZeroCopyOutputStream based on it, simply implement
// CopyingOutputStream and then use CopyingOutputStreamAdaptor.

// CopyingOutputStream implementations should avoid buffering if possible.
// CopyingOutputStreamAdaptor does its own buffering and will write data
// in large locks.
class CopyingOutputStream {
public:
	virtual ~CopyingOutputStream();

	// Writes "size" bytes from the given buffer to the output.  Returns true
	// if successful, false on a write error.
	virtual bool Write(const void* buffer, int size) = 0;
};

// A ZeroCopyingOutputStream which writes to a CopyingOutputStream. This is 
// useful for implementing ZeroCopyOutputStreams that write to traditional 
// streams. Note that this class is not really zero-copy.

// If you want to write to file descriptors or C++ ostreams, this is 
// already implemented for you: use FileOutputStream or OstreamOutputStream 
// respectively.
class CopyingOutputStreamAdaptor : public ZeroCopyOutputStream {
public:
	// Creates a stream that writes to the given Unix file descriptor.
	// If a block_size is given, it specified the size of the buffers 
	// that should be returned by Next(). Otherwise, a reasonable default 
	// is used.
	explicit CopyingOutputStreamAdaptor(CopyingOutputStream* copying_stream,
										int block_size = -1);
	~CopyingOutputStreamAdaptor();

	// Writes all pending data to the underlying stream. Returns false if a 
	// write error occurred on the underlying stream. (The underlying 
	// stream itself is not neccessarily flushed.)
	bool Flush();

	// Call SetOwnsCopyingStream(true) to tell the CopyingOutputStreamAdaptor to 
	// delete the underlying CopyingOutputStream when it is destroyed.
	void SetOwnsCopyingStream(bool value) { owns_copying_stream_ = value; }

	// implements ZeroCopyOutputStream ------------
	bool Next(void**data, int* size);
	void BackUp(int count);
	long ByteCount() const;

private:
	// Write the current buffer, if it is present.
	bool WriteBuffer();
	// Insures that buffer_ is not NULL.
	void AllocateBufferIfNeeded();
	// Frees the buffer.
	void FreeBuffer();

	// The underlying copying stream
	CopyingOutputStream* copying_stream_;
	bool owns_copying_stream_;

	// True if we have seen a permenant error from the underlying stream.
	bool failed_;

	// The current position of copying_stream_, relative to the point where 
	// we started writing.
	long position_;

	// Data is written from this buffer. It may be NULL if no buffer is 
	// currently in use. Otherwise, it points to an array of size buffer_size_.
	scoped_array<uint8_t> buffer_;
	const int buffer_size_;

	// Number of valid bytes currently in the buffer (i.e. the size last
	// returned by Next()). When BackUp() is called, we just reduce this.
	// 0 <= buffer_used_ <= buffer_size_.
	int buffer_used_;

	CopyingOutputStreamAdaptor(const CopyingOutputStreamAdaptor&);
	void operator= (const CopyingOutputStreamAdaptor&);
};

class FileInputStream : public ZeroCopyInputStream
{
public:
	// Creates a stream that reads from the given Unix file descriptor.
	// If a block size is given, it specifies the number of bytes that
	// should be read and returned with each call to Next(). Otherwise,
	// a reasonable default is used.
	explicit FileInputStream(int file_descriptor, int block_size = -1);
	~FileInputStream();

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
	// occurs, the stream is broken and all subsequent operations will
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
	// Creates a stream that writes to the given Unix file descriptor.
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
	// occurs, the stream is broken and all subsequent operations will 
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
