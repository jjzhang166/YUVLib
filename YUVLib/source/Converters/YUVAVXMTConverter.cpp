#include <immintrin.h>
#include "YUVAVXMTConverter.hpp"

#define YKOEF 0, 66, 129, 25
#define UKOEF 0, -38, -74, 112
#define VKOEF 0, 112, -94, -18

namespace YUVLib {

	

	void threadProc(Bitmap * bmp, YUVFrame *frame, const uint32 offsetY, const uint32 linesCount){
		uint32 yStride = frame->GetWidth();
		uint32 uvStride = yStride / 2;
		uint32 rgbStride = bmp->GetWidth() * bmp->GetPixelSize();

		uint8 *currentYLine = frame->GetY() + offsetY * yStride;
		uint8 *uPos = frame->GetU() + offsetY * uvStride / 2;
		uint8 *vPos = frame->GetV() + offsetY * uvStride / 2;

		uint8 *currentLine = bmp->Data() + offsetY * rgbStride;

		uint8 *srcPos, *yPos;

		//
		const __m256i yc = _mm256_set_epi16(YKOEF, YKOEF, YKOEF, YKOEF);
		const __m256i uc = _mm256_set_epi16(UKOEF, UKOEF, UKOEF, UKOEF);
		const __m256i vc = _mm256_set_epi16(VKOEF, VKOEF, VKOEF, VKOEF);
		const __m256i _128 = _mm256_set1_epi16(128);
		const __m256i _16 = _mm256_set1_epi16(16);
		//
		//10000000 00000000 00000000 00000000
		const __m128i srcLoadMask = _mm_set1_epi32(0x80000000);
		//00000000 01000000 00100000 01100000
		const __m256i yuvShuffleMask = _mm256_set1_epi32(0x06040200);
		//10000000 00000000 00000000 00000000
		const __m256i storeMask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0x80000000);

		__m128i line1, line2;
		__m256i lineExt1, lineExt2, y1, y2, u1, u2, v1, v2;

		for (uint32 y = 0; y < linesCount; y += 2) {
			srcPos = currentLine;
			yPos = currentYLine;
			for (uint32 x = 0; x < frame->GetWidth(); x += 4) {
				line1 = _mm_maskload_epi32((int*)(srcPos + rgbStride), srcLoadMask);
				line2 = _mm_maskload_epi32((int*)(srcPos), srcLoadMask);

				lineExt1 = _mm256_cvtepu8_epi16(line1);
				lineExt2 = _mm256_cvtepu8_epi16(line2);

				//Y
				y1 = _mm256_mullo_epi16(lineExt1, yc);
				y2 = _mm256_mullo_epi16(lineExt2, yc);

				y1 = _mm256_hadd_epi16(y1, y1);
				y1 = _mm256_hadd_epi16(y1, y1);

				y2 = _mm256_hadd_epi16(y2, y2);
				y2 = _mm256_hadd_epi16(y2, y2);

				y1 = _mm256_add_epi16(y1, _128);
				y2 = _mm256_add_epi16(y2, _128);

				y1 = _mm256_srli_epi16(y1, 8);
				y2 = _mm256_srli_epi16(y2, 8);

				y1 = _mm256_add_epi16(y1, _16);
				y2 = _mm256_add_epi16(y2, _16);
				//[y11,00,y12,00][y11,00,y12,00][y11,00,y12,00][y11,00,y12,00][y13,00,y14,00][y13,00,y14,00][y13,00,y14,00][y13,00,y14,00]
				//10001000
				y1 = _mm256_permute4x64_epi64(y1, 0x88);
				y2 = _mm256_permute4x64_epi64(y2, 0x88);
				//[y11,00,y12,00][y11,00,y12,00][y13,00,y14,00][y13,00,y14,00][y11,00,y12,00][y11,00,y12,00][y13,00,y14,00][y13,00,y14,00]
				y1 = _mm256_shuffle_epi32(y1, 0x88);
				y2 = _mm256_shuffle_epi32(y2, 0x88);
				//[y11,00,y12,00][y13,00,y14,00][y11,00,y12,00][y13,00,y14,00][y11,00,y12,00][y13,00,y14,00][y11,00,y12,00][y13,00,y14,00]

				y1 = _mm256_shuffle_epi8(y1, yuvShuffleMask);
				y2 = _mm256_shuffle_epi8(y2, yuvShuffleMask);
				//[y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14][y11,y12,y13,y14]
				_mm256_maskstore_epi32((int*)(yPos + yStride), storeMask, y1);
				_mm256_maskstore_epi32((int*)(yPos + 0), storeMask, y2);

				//U
				u1 = _mm256_mullo_epi16(lineExt1, uc);
				u2 = _mm256_mullo_epi16(lineExt2, uc);

				u1 = _mm256_hadd_epi16(u1, u1);
				u2 = _mm256_hadd_epi16(u2, u2);

				u1 = _mm256_hadd_epi16(u1, u1);
				u2 = _mm256_hadd_epi16(u2, u2);

				u1 = _mm256_add_epi16(u1, _128);
				u2 = _mm256_add_epi16(u2, _128);

				u1 = _mm256_srai_epi16(u1, 8);
				u2 = _mm256_srai_epi16(u2, 8);

				u1 = _mm256_add_epi16(u1, _128);
				u2 = _mm256_add_epi16(u2, _128);

				u1 = _mm256_hadd_epi16(u1, u1);
				u2 = _mm256_hadd_epi16(u2, u2);

				u1 = _mm256_add_epi16(u1, u2);

				u1 = _mm256_srli_epi16(u1, 2);

				//u1 = _mm256_permute4x64_epi64(u1, 0x88);
				//u1 = _mm256_shuffle_epi32(u1, 0x88);
				//u1 = _mm256_shuffle_epi8(u1, yuvShuffleMask);

				*(uPos) = (uint8)_mm256_extract_epi8(u1, 0);
				*(uPos + 1) = (uint8)_mm256_extract_epi8(u1, 16);

				//*(uPos + 1) = u1.m256i_u8[16];
				//*(uPos + 0) = u1.m256i_u8[0];

				//V
				v1 = _mm256_mullo_epi16(lineExt1, vc);
				v2 = _mm256_mullo_epi16(lineExt2, vc);

				v1 = _mm256_hadd_epi16(v1, v1);
				v2 = _mm256_hadd_epi16(v2, v2);

				v1 = _mm256_hadd_epi16(v1, v1);
				v2 = _mm256_hadd_epi16(v2, v2);

				v1 = _mm256_add_epi16(v1, _128);
				v2 = _mm256_add_epi16(v2, _128);

				v1 = _mm256_srai_epi16(v1, 8);
				v2 = _mm256_srai_epi16(v2, 8);

				v1 = _mm256_add_epi16(v1, _128);
				v2 = _mm256_add_epi16(v2, _128);

				v1 = _mm256_hadd_epi16(v1, v1);
				v2 = _mm256_hadd_epi16(v2, v2);

				v1 = _mm256_add_epi16(v1, v2);

				v1 = _mm256_srli_epi16(v1, 2);

				//v1 = _mm256_permute4x64_epi64(v1, 0x88);
				//v1 = _mm256_shuffle_epi32(v1, 0x88);
				//v1 = _mm256_shuffle_epi8(v1, yuvShuffleMask);

				*vPos = (uint8)_mm256_extract_epi8(v1, 0);
				*(vPos + 1) = (uint8)_mm256_extract_epi8(v1, 16);

				//*(vPos + 1) = v1.m256i_u8[16];
				//*(vPos + 0) = v1.m256i_u8[0];

				srcPos += 16;
				yPos += 4;
				uPos += 2;
				vPos += 2;
			}
			currentLine += 2 * rgbStride;
			currentYLine += 2 * yStride;
		}
	}

	YUVAVXMTConverter::YUVAVXMTConverter(){
		nProc = std::thread::hardware_concurrency();
		threads = new std::thread[nProc];
	}

	YUVFrame * YUVAVXMTConverter::RGBToYUV420(Bitmap * bmp){
		if (bmp->GetWidth() % 2 != 0 || bmp->GetHeight() % 2 != 0)
			return nullptr;
		YUVFrame *frame = GetFrame(YUVFormats::YUV420);
		frame->Create(bmp->GetWidth(), bmp->GetHeight());
		frame->LockChannels();

		uint32 lines = frame->GetHeight() / nProc;

		if(lines % 2 != 0){
			lines--;
		}

		uint32 offsetY = 0;
		for (uint32 i = 0; i < nProc; i++) {
			if(i == nProc - 1){
				threads[i] = std::thread(threadProc, bmp, frame, offsetY, frame->GetHeight() - offsetY);
			}
			else{
				threads[i] = std::thread(threadProc, bmp, frame, offsetY, lines);
			}
			offsetY += lines;
		}
		for (int i = 0; i < nProc; i++) {
			if (threads[i].joinable()) {
				threads[i].join();
			}
		}
		frame->UnlockChannels();
		return frame;
	}

}