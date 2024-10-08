//----------------------------------------------------------------------------------
// File:        es3-kepler\FXAA\assets\shaders/FXAA_Default.frag
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 0) uniform materialData
{
    vec4 renderTargetSize; //!< Resolution of the processed texture
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (binding = 1) uniform sampler2D scenelightingcolor;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) in vec2 inUV;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) out vec4 outFragColor;

/////////////////////////////////////////////////////////////////////////////////////////////

#define FXAA_QUALITY_PS 12
#define FXAA_QUALITY_P0 1.0
#define FXAA_QUALITY_P1 1.5
#define FXAA_QUALITY_P2 2.0
#define FXAA_QUALITY_P3 2.0
#define FXAA_QUALITY_P4 2.0
#define FXAA_QUALITY_P5 2.0
#define FXAA_QUALITY_P6 2.0
#define FXAA_QUALITY_P7 2.0
#define FXAA_QUALITY_P8 2.0
#define FXAA_QUALITY_P9 2.0
#define FXAA_QUALITY_P10 4.0
#define FXAA_QUALITY_P11 8.0

float FxaaLuma(vec4 rgba) { return rgba.y; }

#define FxaaSat(x) clamp(x, 0.0, 1.0)

#define FxaaTexTop(t, p) textureLod(t, p, 0.0)
#define FxaaTexOff(t, p, o, r) textureLodOffset(t, p, 0.0, o)

vec4 FxaaPixelShader(
    //
    // Use noperspective interpolation here (turn off perspective interpolation).
    // {xy} = center of pixel
    vec2 pos,
    //
    // Used only for FXAA Console, and not used on the 360 version.
    // Use noperspective interpolation here (turn off perspective interpolation).
    // {xy__} = upper left of pixel
    // {__zw} = lower right of pixel
    vec4 fxaaConsolePosPos,
    //
    // Input color texture.
    // {rgb_} = color in linear or perceptual color space
    // if (FXAA_GREEN_AS_LUMA == 0)
    //     {___a} = luma in perceptual color space (not linear)
    sampler2D tex,
    //
    // Only used on FXAA Quality.
    // This must be from a constant/uniform.
    // {x_} = 1.0/screenWidthInPixels
    // {_y} = 1.0/screenHeightInPixels
    vec2 fxaaQualityRcpFrame,
    //
    // Only used on FXAA Quality.
    // This used to be the FXAA_QUALITY__SUBPIX define.
    // It is here now to allow easier tuning.
    // Choose the amount of sub-pixel aliasing removal.
    // This can effect sharpness.
    //   1.00 - upper limit (softer)
    //   0.75 - default amount of filtering
    //   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
    //   0.25 - almost off
    //   0.00 - completely off
    float fxaaQualitySubpix,
    //
    // Only used on FXAA Quality.
    // This used to be the FXAA_QUALITY__EDGE_THRESHOLD define.
    // It is here now to allow easier tuning.
    // The minimum amount of local contrast required to apply algorithm.
    //   0.333 - too little (faster)
    //   0.250 - low quality
    //   0.166 - default
    //   0.125 - high quality 
    //   0.063 - overkill (slower)
    float fxaaQualityEdgeThreshold,
    //
    // Only used on FXAA Quality.
    // This used to be the FXAA_QUALITY__EDGE_THRESHOLD_MIN define.
    // It is here now to allow easier tuning.
    // Trims the algorithm from processing darks.
    //   0.0833 - upper limit (default, the start of visible unfiltered edges)
    //   0.0625 - high quality (faster)
    //   0.0312 - visible limit (slower)
    // Special notes when using FXAA_GREEN_AS_LUMA,
    //   Likely want to set this to zero.
    //   As colors that are mostly not-green
    //   will appear very dark in the green channel!
    //   Tune by looking at mostly non-green content,
    //   then start at zero and increase until aliasing is a problem.
    float fxaaQualityEdgeThresholdMin
)
{
	#define lumaM rgbyM.y

    vec2 posM;
    posM.x                = pos.x;
    posM.y                = pos.y;
    vec4 rgbyM            = FxaaTexTop(tex, posM);
    float lumaS           = FxaaLuma(FxaaTexOff(tex, posM, ivec2( 0, 1), fxaaQualityRcpFrame.xy));
    float lumaE           = FxaaLuma(FxaaTexOff(tex, posM, ivec2( 1, 0), fxaaQualityRcpFrame.xy));
    float lumaN           = FxaaLuma(FxaaTexOff(tex, posM, ivec2( 0,-1), fxaaQualityRcpFrame.xy));
    float lumaW           = FxaaLuma(FxaaTexOff(tex, posM, ivec2(-1, 0), fxaaQualityRcpFrame.xy));
    float maxSM           = max(lumaS, lumaM);
    float minSM           = min(lumaS, lumaM);
    float maxESM          = max(lumaE, maxSM);
    float minESM          = min(lumaE, minSM);
    float maxWN           = max(lumaN, lumaW);
    float minWN           = min(lumaN, lumaW);
    float rangeMax        = max(maxWN, maxESM);
    float rangeMin        = min(minWN, minESM);
    float rangeMaxScaled  = rangeMax * fxaaQualityEdgeThreshold;
    float range           = rangeMax - rangeMin;
    float rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);
    bool earlyExit        = range < rangeMaxClamped;

    if(earlyExit)
    {
        return rgbyM;
    }

    float lumaNW          = FxaaLuma(FxaaTexOff(tex, posM, ivec2(-1,-1), fxaaQualityRcpFrame.xy));
    float lumaSE          = FxaaLuma(FxaaTexOff(tex, posM, ivec2( 1, 1), fxaaQualityRcpFrame.xy));
    float lumaNE          = FxaaLuma(FxaaTexOff(tex, posM, ivec2( 1,-1), fxaaQualityRcpFrame.xy));
    float lumaSW          = FxaaLuma(FxaaTexOff(tex, posM, ivec2(-1, 1), fxaaQualityRcpFrame.xy));
    float lumaNS          = lumaN + lumaS;
    float lumaWE          = lumaW + lumaE;
    float subpixRcpRange  = 1.0/range;
    float subpixNSWE      = lumaNS + lumaWE;
    float edgeHorz1       = (-2.0 * lumaM) + lumaNS;
    float edgeVert1       = (-2.0 * lumaM) + lumaWE;

    float lumaNESE        = lumaNE + lumaSE;
    float lumaNWNE        = lumaNW + lumaNE;
    float edgeHorz2       = (-2.0 * lumaE) + lumaNESE;
    float edgeVert2       = (-2.0 * lumaN) + lumaNWNE;

    float lumaNWSW        = lumaNW + lumaSW;
    float lumaSWSE        = lumaSW + lumaSE;
    float edgeHorz4       = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    float edgeVert4       = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    float edgeHorz3       = (-2.0 * lumaW) + lumaNWSW;
    float edgeVert3       = (-2.0 * lumaS) + lumaSWSE;
    float edgeHorz        = abs(edgeHorz3) + edgeHorz4;
    float edgeVert        = abs(edgeVert3) + edgeVert4;

    float subpixNWSWNESE  = lumaNWSW + lumaNESE;
    float lengthSign      = fxaaQualityRcpFrame.x;
    bool horzSpan         = edgeHorz >= edgeVert;
    float subpixA         = subpixNSWE * 2.0 + subpixNWSWNESE;

    if(!horzSpan)
    {
    	lumaN = lumaW;
    }
    if(!horzSpan)
    {
    	lumaS = lumaE;
    }
    if(horzSpan)
    {
    	lengthSign = fxaaQualityRcpFrame.y;
    }

    float subpixB   = (subpixA * (1.0 / 12.0)) - lumaM;
    float gradientN = lumaN - lumaM;
    float gradientS = lumaS - lumaM;
    float lumaNN    = lumaN + lumaM;
    float lumaSS    = lumaS + lumaM;
    bool pairN      = abs(gradientN) >= abs(gradientS);
    float gradient  = max(abs(gradientN), abs(gradientS));

    if(pairN)
    {
    	lengthSign = -lengthSign;
    }

    float subpixC = FxaaSat(abs(subpixB) * subpixRcpRange);

    vec2 posB;
    vec2 offNP;
    posB.x  = posM.x;
    posB.y  = posM.y;
    offNP.x = (!horzSpan) ? 0.0 : fxaaQualityRcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : fxaaQualityRcpFrame.y;

    if(!horzSpan)
    {
    	posB.x += lengthSign * 0.5;
    }
    if(horzSpan)
    {
    	posB.y += lengthSign * 0.5;
    }

    vec2 posN;
    vec2 posP;
    posN.x         = posB.x - offNP.x * FXAA_QUALITY_P0;
    posN.y         = posB.y - offNP.y * FXAA_QUALITY_P0;
    posP.x         = posB.x + offNP.x * FXAA_QUALITY_P0;
    posP.y         = posB.y + offNP.y * FXAA_QUALITY_P0;
    float subpixD  = ((-2.0)*subpixC) + 3.0;
    float lumaEndN = FxaaLuma(FxaaTexTop(tex, posN));
    float subpixE  = subpixC * subpixC;
    float lumaEndP = FxaaLuma(FxaaTexTop(tex, posP));

    if(!pairN)
    {
    	lumaNN = lumaSS;
    }

    float gradientScaled = gradient * 1.0/4.0;
    float lumaMM         = lumaM - lumaNN * 0.5;
    float subpixF        = subpixD * subpixE;
    bool lumaMLTZero     = lumaMM < 0.0;

    lumaEndN            -= lumaNN * 0.5;
    lumaEndP            -= lumaNN * 0.5;
    bool doneN           = abs(lumaEndN) >= gradientScaled;
    bool doneP           = abs(lumaEndP) >= gradientScaled;

    if(!doneN)
    {
    	posN.x -= offNP.x * FXAA_QUALITY_P1;
    }
    if(!doneN)
    {
    	posN.y -= offNP.y * FXAA_QUALITY_P1;
    }

    bool doneNP = (!doneN) || (!doneP);

    if(!doneP)
    {
    	posP.x += offNP.x * FXAA_QUALITY_P1;
    }
    if(!doneP)
    {
    	posP.y += offNP.y * FXAA_QUALITY_P1;
    }

    if(doneNP)
    {
        if(!doneN)
        {
        	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        }
        if(!doneP)
        {
        	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        }
        if(!doneN)
        {
        	lumaEndN = lumaEndN - lumaNN * 0.5;
        }
        if(!doneP)
        {
        	lumaEndP = lumaEndP - lumaNN * 0.5;
        }

        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;

        if(!doneN)
        {
        	posN.x -= offNP.x * FXAA_QUALITY_P2;
        }
        if(!doneN)
        {
        	posN.y -= offNP.y * FXAA_QUALITY_P2;
        }

        doneNP = (!doneN) || (!doneP);

        if(!doneP)
        {
        	posP.x += offNP.x * FXAA_QUALITY_P2;
        }
        if(!doneP)
        {
        	posP.y += offNP.y * FXAA_QUALITY_P2;
        }

        if(doneNP)
        {
            if(!doneN)
            {
            	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
            }
            if(!doneP)
            {
            	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
            }
            if(!doneN)
            {
            	lumaEndN = lumaEndN - lumaNN * 0.5;
            }
            if(!doneP)
            {
            	lumaEndP = lumaEndP - lumaNN * 0.5;
            }

            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;

            if(!doneN)
            {
            	posN.x -= offNP.x * FXAA_QUALITY_P3;
            }
            if(!doneN)
            {
            	posN.y -= offNP.y * FXAA_QUALITY_P3;
            }

            doneNP = (!doneN) || (!doneP);

            if(!doneP)
            {
            	posP.x += offNP.x * FXAA_QUALITY_P3;
            }
            if(!doneP)
            {
            	posP.y += offNP.y * FXAA_QUALITY_P3;
            }

            if(doneNP)
            {
                if(!doneN)
                {
                	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                }
                if(!doneP)
                {
                	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                }
                if(!doneN)
                {
                	lumaEndN = lumaEndN - lumaNN * 0.5;
                }
                if(!doneP)
                {
                	lumaEndP = lumaEndP - lumaNN * 0.5;
                }

                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;

                if(!doneN)
                {
                	posN.x -= offNP.x * FXAA_QUALITY_P4;
                }
                if(!doneN)
                {
                	posN.y -= offNP.y * FXAA_QUALITY_P4;
                }

                doneNP = (!doneN) || (!doneP);

                if(!doneP)
                {
                	posP.x += offNP.x * FXAA_QUALITY_P4;
                }
                if(!doneP)
                {
                	posP.y += offNP.y * FXAA_QUALITY_P4;
                }

                if(doneNP)
                {
                    if(!doneN)
                    {
                    	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                    }
                    if(!doneP)
                    {
                    	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                    }
                    if(!doneN)
                    {
                    	lumaEndN = lumaEndN - lumaNN * 0.5;
                    }
                    if(!doneP)
                    {
                    	lumaEndP = lumaEndP - lumaNN * 0.5;
                    }

                    doneN = abs(lumaEndN) >= gradientScaled;
                    doneP = abs(lumaEndP) >= gradientScaled;

                    if(!doneN)
                    {
                    	posN.x -= offNP.x * FXAA_QUALITY_P5;
                    }
                    if(!doneN)
                    {
                    	posN.y -= offNP.y * FXAA_QUALITY_P5;
                    }

                    doneNP = (!doneN) || (!doneP);

                    if(!doneP)
                    {
                    	posP.x += offNP.x * FXAA_QUALITY_P5;
                    }
                    if(!doneP)
                    {
                    	posP.y += offNP.y * FXAA_QUALITY_P5;
                    }

                    if(doneNP)
                    {
                        if(!doneN)
                        {
                        	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                        }
                        if(!doneP)
                        {
                        	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                        }
                        if(!doneN)
                        {
                        	lumaEndN = lumaEndN - lumaNN * 0.5;
                        }
                        if(!doneP)
                        {
                        	lumaEndP = lumaEndP - lumaNN * 0.5;
                        }

                        doneN = abs(lumaEndN) >= gradientScaled;
                        doneP = abs(lumaEndP) >= gradientScaled;

                        if(!doneN)
                        {
                        	posN.x -= offNP.x * FXAA_QUALITY_P6;
                        }
                        if(!doneN)
                        {
                        	posN.y -= offNP.y * FXAA_QUALITY_P6;
                        }

                        doneNP = (!doneN) || (!doneP);

                        if(!doneP)
                        {
                        	posP.x += offNP.x * FXAA_QUALITY_P6;
                        }
                        if(!doneP)
                        {
                        	posP.y += offNP.y * FXAA_QUALITY_P6;
                        }

                        if(doneNP)
                        {
                            if(!doneN)
                            {
                            	lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                            }
                            if(!doneP)
                            {
                            	lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                            }
                            if(!doneN)
                            {
                            	lumaEndN = lumaEndN - lumaNN * 0.5;
                            }
                            if(!doneP)
                            {
                            	lumaEndP = lumaEndP - lumaNN * 0.5;
                            }

                            doneN = abs(lumaEndN) >= gradientScaled;
                            doneP = abs(lumaEndP) >= gradientScaled;

                            if(!doneN)
                            {
                            	posN.x -= offNP.x * FXAA_QUALITY_P7;
                            }
                            if(!doneN)
                            {
                            	posN.y -= offNP.y * FXAA_QUALITY_P7;
                            }

                            doneNP = (!doneN) || (!doneP);

                            if(!doneP)
                            {
                            	posP.x += offNP.x * FXAA_QUALITY_P7;
                            }
                            if(!doneP)
                            {
                            	posP.y += offNP.y * FXAA_QUALITY_P7;
                            }

    						if(doneNP)
    						{
        						if(!doneN)
        						{
        							lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        						}
        						if(!doneP)
        						{
        							lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        						}
        						if(!doneN)
        						{
        							lumaEndN = lumaEndN - lumaNN * 0.5;
        						}
        						if(!doneP)
        						{
        							lumaEndP = lumaEndP - lumaNN * 0.5;
        						}

        						doneN = abs(lumaEndN) >= gradientScaled;
        						doneP = abs(lumaEndP) >= gradientScaled;

        						if(!doneN)
        						{
        							posN.x -= offNP.x * FXAA_QUALITY_P8;
        						}
        						if(!doneN)
        						{
        							posN.y -= offNP.y * FXAA_QUALITY_P8;
        						}

        						doneNP = (!doneN) || (!doneP);

        						if(!doneP)
        						{
        							posP.x += offNP.x * FXAA_QUALITY_P8;
        						}
        						if(!doneP)
        						{
        							posP.y += offNP.y * FXAA_QUALITY_P8;
        						}

        						if(doneNP)
        						{
            						if(!doneN)
            						{
            							lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
            						}
            						if(!doneP)
            						{
            							lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
            						}
            						if(!doneN)
            						{
            							lumaEndN = lumaEndN - lumaNN * 0.5;
            						}
            						if(!doneP)
            						{
            							lumaEndP = lumaEndP - lumaNN * 0.5;
            						}

            						doneN = abs(lumaEndN) >= gradientScaled;
            						doneP = abs(lumaEndP) >= gradientScaled;

            						if(!doneN)
            						{
            							posN.x -= offNP.x * FXAA_QUALITY_P9;
            						}
            						if(!doneN)
            						{
            							posN.y -= offNP.y * FXAA_QUALITY_P9;
            						}

            						doneNP = (!doneN) || (!doneP);

            						if(!doneP)
            						{
            							posP.x += offNP.x * FXAA_QUALITY_P9;
            						}
            						if(!doneP)
            						{
            							posP.y += offNP.y * FXAA_QUALITY_P9;
            						}

            						if(doneNP)
            						{
                						if(!doneN)
                						{
                							lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                						}
                						if(!doneP)
                						{
                							lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                						}
                						if(!doneN)
                						{
                							lumaEndN = lumaEndN - lumaNN * 0.5;
                						}
                						if(!doneP)
                						{
                							lumaEndP = lumaEndP - lumaNN * 0.5;
                						}

                						doneN = abs(lumaEndN) >= gradientScaled;
                						doneP = abs(lumaEndP) >= gradientScaled;

                						if(!doneN)
                						{
                							posN.x -= offNP.x * FXAA_QUALITY_P10;
                						}
                						if(!doneN)
                						{
                							posN.y -= offNP.y * FXAA_QUALITY_P10;
                						}

                						doneNP = (!doneN) || (!doneP);

                						if(!doneP)
                						{
                							posP.x += offNP.x * FXAA_QUALITY_P10;
                						}
                						if(!doneP)
                						{
                							posP.y += offNP.y * FXAA_QUALITY_P10;
                						}

                						if(doneNP)
                						{
                    						if(!doneN)
                    						{
                    							lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                    						}
                    						if(!doneP)
                    						{
                    							lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                    						}
                    						if(!doneN)
                    						{
                    							lumaEndN = lumaEndN - lumaNN * 0.5;
                    						}
                    						if(!doneP)
                    						{
                    							lumaEndP = lumaEndP - lumaNN * 0.5;
                    						}

                    						doneN = abs(lumaEndN) >= gradientScaled;
                    						doneP = abs(lumaEndP) >= gradientScaled;

                    						if(!doneN)
                    						{
                    							posN.x -= offNP.x * FXAA_QUALITY_P11;
                    						}
                    						if(!doneN)
                    						{
                    							posN.y -= offNP.y * FXAA_QUALITY_P11;
                    						}

                    						doneNP = (!doneN) || (!doneP);

                    						if(!doneP)
                    						{
                    							posP.x += offNP.x * FXAA_QUALITY_P11;
                    						}
                    						if(!doneP)
                    						{
                    							posP.y += offNP.y * FXAA_QUALITY_P11;
                    						}
                						}
            						}
        						}
    						}
                        }
                    }
                }
            }
        }
    }

    float dstN = posM.x - posN.x;
    float dstP = posP.x - posM.x;

    if(!horzSpan)
    {
    	dstN = posM.y - posN.y;
    }
    if(!horzSpan)
    {
    	dstP = posP.y - posM.y;
    }

    bool goodSpanN          = (lumaEndN < 0.0) != lumaMLTZero;
    float spanLength        = (dstP + dstN);
    bool goodSpanP          = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLengthRcp     = 1.0 / spanLength;

    bool directionN         = dstN < dstP;
    float dst               = min(dstN, dstP);
    bool goodSpan           = directionN ? goodSpanN : goodSpanP;
    float subpixG           = subpixF * subpixF;
    float pixelOffset       = (dst * (-spanLengthRcp)) + 0.5;
    float subpixH           = subpixG * fxaaQualitySubpix;

    float pixelOffsetGood   = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);

    if(!horzSpan)
    {
    	posM.x += pixelOffsetSubpix * lengthSign;
    }
    if( horzSpan)
    {
    	posM.y += pixelOffsetSubpix * lengthSign;
    }

    return vec4(FxaaTexTop(tex, posM).xyz, lumaM);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main(void)
{
    vec2 textureSize = myMaterialData.renderTargetSize.xy;
    outFragColor     = FxaaPixelShader(inUV,
		                               vec4(0.0f, 0.0f, 0.0f, 0.0f),                   // vec4 fxaaConsolePosPos,
                                       scenelightingcolor,                             // sampler2D tex,
                                       vec2(1.0 / textureSize.x, 1.0 / textureSize.y), // vec2 fxaaQualityRcpFrame,
                                       0.75,                                           // float fxaaQualitySubpix,
                                       0.166,                                          // float fxaaQualityEdgeThreshold,
                                       0.0833);                                        // float fxaaQualityEdgeThresholdMin
}

/////////////////////////////////////////////////////////////////////////////////////////////
