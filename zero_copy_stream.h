

#ifndef __CCC_ZERO_COPY_STREAM_H__
#define __CCC_ZERO_COPY_STREAM_H__

#include <string>
#include "common.h"

class ZeroCopyInputStream
{
public:
	ZeroCopyInputStream() {}
	virtual ~ZeroCopyInputStream();

	// Obtains a chunk of data from the stream
	// Preconditions:
	// * "size" and "data" are not NULL
	// 
	// Postconditions:
	// * If the returned value is false, there is no more data to return or
	//   an error occurred. All errors are permanent.
	// * Otherwise, "size" points to the actual number of bytes read and "data"
	//   points to a pointer to a buffer containing these bytes.
	// * Ownership of this buffer remians with the stream, and the buffer
	//   remains valid only until some other method of the stream is called
	//   or the stream is destroyed
	// * It is legal for the returned buffer to have zero size, as long 
	//   as repeatedly calling Next() eventually yeilds a buffer with non-zero
	//   size.
	virtual bool Next(const void** data, int* size) = 0;

	// Backs up a number of bytes, so that the next call to Next() returns 
	// data again that was already returned by the last call the Next(). This
	// is useful when writing procedures that are only supposed to read up 
	// to a certain point in the input, then return. If Next() returns a 
	// buffer that goes beyond what you wanted to read, you can use BackUp()
	// to return to the point where you intended to finish.
	// 
	// Preconditioons:
	// * The last method called must have been Next().
	// * count must be less than or equal to the size of the last buffer 
	//   returned by Next().
	//   
	// Postconditions:
	// * The last "count" bytes of the last buffer returned by Next() will be
	//   pushed back into the stream. Subsequent calls to Next() will return 
	//   the same data again before producing new data.
	virtual void BackUp(int count) = 0;

	// Skips a number of bytes. Returns false if the end of the steram is
	// reached or some input error occurred. In the end-of-stream case, the 
	// steram is advanced to the end of the stream (so ByteCount() will return
	// the total size of the steram).
	virtual bool Skip(int count) = 0;

	// Returns the total number of bytes read since this object was created.
	virtual long ByteCount() const = 0;

private:
	/// DISALLOW_EVIL_CONSTRUCTORS(TypeName)
	ZeroCopyInputStream(const ZeroCopyInputStream& );
	void operator=(const ZeroCopyInputStream& );
};


// Astract interface similar to an output stream but designed to minimize 
// copying.
class ZeroCopyOutputStream {
public:
	inline ZeroCopyOutputStream() {}
	virtual ~ZeroCopyOutputStream();

	// Obtains a buffer into which data can written. Any data written
	// into this buffer will eventually (maybe instantly, maybe later on)
	// be written to the output.

	// Preconditions:
	// * "size" and "data" are not NULL.

	// Postconditions:
	// * If the returned value is false, an error occurred. All errors are
	//   permanent
	// * Otherwise, "size" points to the actual number of bytes in the buffer
	//   and "data" points to the buffer.
	// * Ownership of this buffer remains with the stream, and the buffer
	//   remains valid only until some other method of the stream is called
	// * Any data which the caller stores in this buffer will eventually be
	//   written to the output (unless BackUp() is called).
	// * It is legal for the returned buffer to have zero size, as long 
	//   as repeatedly calling Next() eventually yields a buffer with non-zero
	//   size.
	virtual bool Next(void** data, int* size) = 0;

	// backs up a number of bytes, so that the end of the last buffer returned
	// by Next() is not actually written. This is needed when you finish
	// writting all the data you want to write, but the last buffer was bigger 
	// than you needed. You don't want to write a bunch of garbage after the 
	// end of your data, so you use BackUp() to back up.

	// Preconditions:
	// * The last method called must have been Next().
	// * count must be less than or equal to the size of the last buffer 
	//   returned by Next().
	// * The caller must not have written anything to the last "count" bytes 
	//   of that buffer.

	// Postconditions:
	// * The last "count" bytes of the last buffer returned by Next() will be 
	//   Ignored.
	virtual void BackUp(int count) = 0;

	// Returns the total number of bytes written since this object was created.
	virtual long ByteCount() const = 0;

	// Write a given chunk of data to the output. Some output streams may
	// implement this in a way that avoids coping. Check AllowsAliasing() before 
	// calling WriteAliasedRaw(). It will GOOGLE_CHECK fail if WriteAliasedRaw() is
	// called on a stream that does not allow aliasing.

	// NOTE: It is caller's responsibility to ensure that the chunk of memory
	// remains live until all of the data has been consumed from the stream.
	virtual bool WriteAliasedRaw(const void* data, int size);
	virtual bool AllowsAliasing() const { return false; }

private:
	ZeroCopyOutputStream(const ZeroCopyOutputStream&);
	void operator= (const ZeroCopyOutputStream&);
};

#endif /// __CCC_ZERO_COPY_STREAM_H__
