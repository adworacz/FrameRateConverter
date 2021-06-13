#include "ContinuousMask.h"

ContinuousMask::ContinuousMask(PClip _child, int _radius, IScriptEnvironment* env) :
	GenericVideoFilter(_child), radius(_radius) {

	if (!vi.IsYUV() && !vi.IsY())
	if (radius <= 1)
		env->ThrowError("ContinuousMask: Radius must be above 1");
}

ContinuousMask::~ContinuousMask() {
}

PVideoFrame __stdcall ContinuousMask::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (vi.BitsPerComponent() == 8)
		Calculate<uint16_t, BYTE>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	else if (vi.BitsPerComponent() < 32)
		Calculate<uint32_t, uint16_t>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	else
		Calculate<float, float>(src->GetReadPtr(), src->GetPitch(), dst->GetWritePtr(), dst->GetPitch(), env);
	return dst;
}

// T: data type to calculate total (must hold P.MaxValue * radius * 4)
// P: data type of each pixel
template<typename T, typename P> void ContinuousMask::Calculate(const BYTE* srcp, int srcPitch, BYTE* dstp, int dstPitch, IScriptEnvironment* env) {
	int width = vi.width;
	int height = vi.height;
	memset(dstp, 0, dstPitch * height);
	T Sum = 0;
	const P* srcIter = (const P*)srcp;
	P* dstIter = (P*)dstp;
	int radFwd, radBck;
	int radFwdV, radBckV;
	srcPitch = srcPitch / sizeof(P);
	dstPitch = dstPitch / sizeof(P);

	// Calculate the average of [radius] pixels in all 4 directions, for source pixels having a value.
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (srcIter[x] > 0) {
				Sum = 0;
				radFwd = min(radius, width - x);
				radBck = min(min(radius, x + 1), width) - 1;
				radFwdV = min(radius, height - y);
				radBckV = min(min(radius, y + 1), height) - 1;
				for (int i = -radBck; i < radFwd; i++) {
					Sum += (T)srcIter[x + i];
				}
				for (int i = -radBckV; i < radFwdV; i++) {
					Sum += (T)srcIter[x + i * srcPitch];
				}
				dstIter[x] = P(Sum / (radFwd + radBck + radFwdV + radBckV));
			}
		}
		srcIter += srcPitch;
		dstIter += dstPitch;
	}
}

// Marks filter as multi-threading friendly.
int __stdcall ContinuousMask::SetCacheHints(int cachehints, int frame_range) {
	return cachehints == CachePolicyHint::CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}
