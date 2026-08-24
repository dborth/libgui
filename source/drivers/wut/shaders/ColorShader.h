/****************************************************************************
 * libgui - drivers/wut/shaders
 * Daryl Borth 2026
 * ColorShader.h
 ***************************************************************************/
#ifndef WUT_COLOR_SHADER_H_
#define WUT_COLOR_SHADER_H_

#include "VertexShader.h"
#include "PixelShader.h"
#include "FetchShader.h"

class ColorShader : public Shader
{
	private:
		ColorShader();
		virtual ~ColorShader();

		static const uint32_t cuAttributeCount = 2;
		static const uint32_t cuPositionVtxsSize = 4 * cuVertexAttrSize;

		static ColorShader * shaderInstance;

		FetchShader * fetchShader;
		VertexShader vertexShader;
		PixelShader pixelShader;

		float * positionVtxs;
		uint8_t * colorVtxs;

		uint32_t angleLocation;
		uint32_t offsetLocation;
		uint32_t scaleLocation;
		uint32_t colorLocation;
		uint32_t colorIntensityLocation;
		uint32_t positionLocation;

	public:
		static const uint32_t cuColorVtxsSize = 4 * cuColorAttrSize;

		static ColorShader * instance()
		{
			if(!shaderInstance)
				shaderInstance = new ColorShader();
			return shaderInstance;
		}

		static void destroyInstance()
		{
			delete shaderInstance;
			shaderInstance = nullptr;
		}

		void setShaders() const
		{
			fetchShader->setShader();
			vertexShader.setShader();
			pixelShader.setShader();
		}

		//!\param colorAttr Packed RGBA8 color, 4 vertices (cuColorVtxsSize bytes).
		//!Copied into a buffer this shader owns for the GPU to read - the
		//!caller's memory (a stack array at every current call site) isn't
		//!guaranteed to still be valid, unmodified, or cache-flushed by the
		//!time the GPU actually executes this draw.
		void setAttributeBuffer(const uint8_t * colorAttr) const
		{
			VertexShader::setAttributeBuffer(0, cuPositionVtxsSize, cuVertexAttrSize, positionVtxs);

			if(colorVtxs)
			{
				memcpy(colorVtxs, colorAttr, cuColorVtxsSize);
				GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, colorVtxs, cuColorVtxsSize);
			}
			VertexShader::setAttributeBuffer(1, cuColorVtxsSize, cuColorAttrSize, colorVtxs);
		}

		void setAngle(float angleRadians)
		{
			VertexShader::setUniformReg(angleLocation, 4, &angleRadians);
		}
		void setOffset(const float offset[3])
		{
			VertexShader::setUniformReg(offsetLocation, 4, offset);
		}
		void setScale(const float scale[3])
		{
			VertexShader::setUniformReg(scaleLocation, 4, scale);
		}
		void setColorIntensity(const float colorIntensity[4])
		{
			PixelShader::setUniformReg(colorIntensityLocation, 4, colorIntensity);
		}
};

#endif // WUT_COLOR_SHADER_H_
