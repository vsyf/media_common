/*
 * media_source.h
 * Copyright (C) 2022 youfa.song <vsyfar@gmail.com>
 *
 * Distributed under terms of the GPLv2 license.
 */

#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H

#include <memory>
#include <vector>

#include "common/media_errors.h"
#include "common/message.h"
#include "common/meta_data.h"

namespace avp {

class MediaSource : public MessageObject {
 public:
  // Options that modify read() behaviour. The default is to
  // a) not request a seek
  // b) not be late, i.e. lateness_us = 0
  struct ReadOptions {
    enum SeekMode : int32_t {
      SEEK_PREVIOUS_SYNC,
      SEEK_NEXT_SYNC,
      SEEK_CLOSEST_SYNC,
      SEEK_CLOSEST,
    };

    ReadOptions();

    // Reset everything back to defaults.
    void reset();

    void setSeekTo(int64_t time_us, SeekMode mode = SEEK_CLOSEST_SYNC);
    void clearSeekTo();
    bool getSeekTo(int64_t* time_us, SeekMode* mode) const;

    // TODO: remove this if unused.
    void setLateBy(int64_t lateness_us);
    int64_t getLateBy() const;

    void setNonBlocking();
    void clearNonBlocking();
    bool getNonBlocking() const;

    // Used to clear all non-persistent options for multiple buffer reads.
    void clearNonPersistent() { clearSeekTo(); }

   private:
    enum Options {
      kSeekTo_Option = 1,
    };

    uint32_t mOptions;
    int64_t mSeekTimeUs;
    SeekMode mSeekMode;
    int64_t mLatenessUs;
    bool mNonBlocking;
  } __attribute__((packed));  // sent through Binder

  MediaSource() = default;
  virtual ~MediaSource() = default;

  // To be called before any other methods on this object, except
  // getFormat().
  virtual status_t start(MetaData* params = nullptr) = 0;

  // Any blocking read call returns immediately with a result of NO_INIT.
  // It is an error to call any methods other than start after this call
  // returns. Any buffers the object may be holding onto at the time of
  // the stop() call are released.
  // Also, it is imperative that any buffers output by this object and
  // held onto by callers be released before a call to stop() !!!
  virtual status_t stop() = 0;

  // Returns the format of the data output by this media source.
  virtual std::shared_ptr<MetaData> getFormat() = 0;

  // Returns a new buffer of data. Call blocks until a
  // buffer is available, an error is encountered of the end of the stream
  // is reached.
  // End of stream is signalled by a result of ERROR_END_OF_STREAM.
  // A result of INFO_FORMAT_CHANGED indicates that the format of this
  // MediaSource has changed mid-stream, the client can continue reading
  // but should be prepared for buffers of the new configuration.
  virtual status_t read(std::shared_ptr<Buffer>& buffer,
                        const ReadOptions* options = nullptr) = 0;

  // Causes this source to suspend pulling data from its upstream source
  // until a subsequent read-with-seek. This is currently not supported
  // as such by any source. E.g. MediaCodecSource does not suspend its
  // upstream source, and instead discard upstream data while paused.
  virtual status_t pause() { return ERROR_UNSUPPORTED; }

  // The consumer of this media source requests that the given buffers
  // are to be returned exclusively in response to read calls.
  // This will be called after a successful start() and before the
  // first read() call.
  // Callee assumes ownership of the buffers if no error is returned.
  virtual status_t setBuffers(const std::vector<Buffer*>& /* buffers */) {
    return ERROR_UNSUPPORTED;
  }

  // The consumer of this media source requests the source stops sending
  // buffers with timestamp larger than or equal to stopTimeUs. stopTimeUs
  // must be in the same time base as the startTime passed in start(). If
  // source does not support this request, ERROR_UNSUPPORTED will be returned.
  // If stopTimeUs is invalid, BAD_VALUE will be returned. This could be
  // called at any time even before source starts and it could be called
  // multiple times. Setting stopTimeUs to be -1 will effectively cancel the
  // stopTimeUs set previously. If stopTimeUs is larger than or equal to last
  // buffer's timestamp, source will start to drop buffer when it gets a buffer
  // with timestamp larger than or equal to stopTimeUs. If stopTimeUs is smaller
  // than or equal to last buffer's timestamp, source will drop all the incoming
  // buffers immediately. After setting stopTimeUs, source may still stop
  // sending buffers with timestamp less than stopTimeUs if it is stopped by the
  // consumer.
  virtual status_t setStopTimeUs(int64_t /* stopTimeUs */) {
    return ERROR_UNSUPPORTED;
  }

 private:
  /* data */

  AVP_DISALLOW_COPY_AND_ASSIGN(MediaSource);
};

using ReadOptions = MediaSource::ReadOptions;

}  // namespace avp

#endif /* !MEDIA_SOURCE_H */
