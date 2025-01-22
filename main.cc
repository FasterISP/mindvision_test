#include <SDL2/SDL.h>
#include <stdio.h>

#include <chrono>
#include <iostream>

#include "mvcamera/CameraApi.h"
#include "mvcamera/CameraDefine.h"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/highgui/highgui.hpp"
using namespace cv;

unsigned char *g_pRgbBuffer;  // 处理后数据缓存区

int main() {
  SDL_Event event;
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;

  int iCameraCounts = 1;
  int iStatus = -1;
  tSdkCameraDevInfo tCameraEnumList;
  int hCamera;
  tSdkCameraCapbility tCapability;
  tSdkFrameHead sFrameInfo;
  BYTE *pbyBuffer;
  int iDisplayFrames = 10000;
  IplImage *iplImage = NULL;
  int channel = 3;

  CameraSdkInit(1);

  iStatus = CameraEnumerateDevice(&tCameraEnumList, &iCameraCounts);
  printf("state = %d, count=%d\n", iStatus, iCameraCounts);
  if (iCameraCounts == 0) {
    return -1;
  }

  iStatus = CameraInit(&tCameraEnumList, -1, -1, &hCamera);

  printf("state = %d\n", iStatus);
  if (iStatus != CAMERA_STATUS_SUCCESS) {
    return -1;
  }

  CameraGetCapability(hCamera, &tCapability);

  g_pRgbBuffer =
      (unsigned char *)malloc(tCapability.sResolutionRange.iHeightMax *
                              tCapability.sResolutionRange.iWidthMax * 3);
  CameraPlay(hCamera);

  if (tCapability.sIspCapacity.bMonoSensor) {
    channel = 1;
    CameraSetIspOutFormat(hCamera, CAMERA_MEDIA_TYPE_MONO8);
  } else {
    channel = 3;
    CameraSetIspOutFormat(hCamera, CAMERA_MEDIA_TYPE_RGB8);
  }

  CameraSetExposureTime(hCamera, 50*1000);
  CameraSetTriggerMode(
      hCamera, 1);  // 0表示连续采集模式；1表示软件触发模式；2表示硬件触发模式
  CameraSetTriggerCount(hCamera, 1);
  CameraSetExtTrigJitterTime(hCamera, 0);
  CameraSetExtTrigIntervalTime(hCamera, 0);
  CameraSetExtTrigDelayTime(hCamera, 0);
  CameraSetExtTrigBufferedDelayTime(hCamera, 0);

  while (iDisplayFrames--) {
    auto trigger_start_time = std::chrono::system_clock::now();
    CameraSoftTriggerEx(hCamera, CAMERA_ST_CLEAR_BUFFER_BEFORE);
    std::cout << "trigger time cost(" << iDisplayFrames << "): "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now() - trigger_start_time)
                     .count()
              << " ms" << std::endl;

    auto capture_start_time = std::chrono::system_clock::now();
    if (CameraGetImageBuffer(hCamera, &sFrameInfo, &pbyBuffer, 100) ==
        CAMERA_STATUS_SUCCESS) {
      std::cout << "capture time cost(" << iDisplayFrames << "): "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - capture_start_time)
                       .count()
                << " ms" << std::endl;
      if (window == NULL) {
        window = SDL_CreateWindow("show", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, sFrameInfo.iWidth,
                                  sFrameInfo.iHeight, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    sFrameInfo.iWidth, sFrameInfo.iHeight);
      }

      auto isp_start_time = std::chrono::system_clock::now();
      CameraImageProcess(hCamera, pbyBuffer, g_pRgbBuffer, &sFrameInfo);
      std::cout << "ISP time cost(" << iDisplayFrames << "): "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - isp_start_time)
                       .count()
                << " ms" << std::endl;

      auto sdl_start_time = std::chrono::system_clock::now();
      SDL_UpdateTexture(texture, NULL, g_pRgbBuffer, sFrameInfo.iWidth * 3);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);

      SDL_PollEvent(&event);
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q) {
        break;
      }

      std::cout << "SDL time cost(" << iDisplayFrames << "): "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - sdl_start_time)
                       .count()
                << " ms" << std::endl
                << std::endl;

      CameraReleaseImageBuffer(hCamera, pbyBuffer);
    }
  }

  CameraUnInit(hCamera);

  free(g_pRgbBuffer);

  if (texture != nullptr) {
    SDL_DestroyTexture(texture);
  }
  if (renderer != nullptr) {
    SDL_DestroyRenderer(renderer);
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  return 0;
}
