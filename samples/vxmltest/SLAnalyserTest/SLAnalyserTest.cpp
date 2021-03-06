// SLAnalyserTest.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#if defined(_WIN32)
  #define SL_EXPORT __declspec(dllexport)
#endif

#include <ptclib/SignLanguageAnalyser.h>

#include <queue>
#include <vector>
#include <mutex>



struct VideoFrame
{
  const unsigned m_width;
  const unsigned m_height;
  const std::vector<char> m_pixels;

  VideoFrame(unsigned width, unsigned height, const char * pixels)
    : m_width(width)
    , m_height(height)
    , m_pixels(pixels, pixels+width*height*3/2)
  {
  }
};

static std::queue<VideoFrame> s_videoQueue;
static std::mutex s_mutex;

extern "C" {
  SL_EXPORT int SL_STDCALL SLInitialise(SLAnalyserInit * init)
  {
    if (!init)
      return -1;

    init->m_apiVersion = SL_API_VERSION;
    init->m_videoFormat = SL_YUV420P;
    init->m_maxInstances = 1;
    return 0;
  }


  SL_EXPORT int SL_STDCALL SLRelease()
  {
    s_mutex.lock();
    while (!s_videoQueue.empty())
      s_videoQueue.pop();
    s_mutex.unlock();
    return 0;
  }


  SL_EXPORT int SL_STDCALL SLAnalyse(const SLAnalyserData * data)
  {
    if (!data || data->m_width == 0 || data->m_height == 0 || data->m_instance != 0)
      return -1;

    s_mutex.lock();
    s_videoQueue.emplace(VideoFrame(data->m_width, data->m_height, static_cast<const char *>(data->m_pixels)));
    if (s_videoQueue.size() > 10)
      s_videoQueue.pop();
    s_mutex.unlock();

    return 0;
  }


  SL_EXPORT int SL_STDCALL SLPreview(const SLPreviewData * data)
  {
    if (!data || data->m_instance != 0)
      return -1;

    int result;
    s_mutex.lock();

    if (s_videoQueue.empty())
      result = 0;
    else {
      const VideoFrame & frame = s_videoQueue.front();
      if (data->m_width != frame.m_width || data->m_height != frame.m_height)
        result = -2;
      else {
        memcpy(data->m_pixels, frame.m_pixels.data(), frame.m_pixels.size());
        result = 1;
      }

      s_videoQueue.pop();
    }

    s_mutex.unlock();

    return result;
  }

};
