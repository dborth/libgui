/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiImageData.cpp
 ***************************************************************************/

#include "Gui.h"

GuiImageData::GuiImageData(const uint8_t * i, int maxw, int maxh)
{
	data = nullptr;
	texture = nullptr;
	width = 0;
	height = 0;

	if(i)
		data = (uint8_t *)platform->getVideo()->getImageRenderer()->decodeImage(i, &width, &height, maxw, maxh);

	if(data)
		texture = platform->getVideo()->getImageRenderer()->createTexture(data, width, height);
}

GuiImageData::~GuiImageData()
{
	if(texture)
	{
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);
		texture = nullptr;
	}

	if(data)
	{
		free(data);
		data = nullptr;
	}
}

uint8_t * GuiImageData::getImage()
{
	return data;
}

void * GuiImageData::getTexture()
{
	return texture;
}

int GuiImageData::getWidth()
{
	return width;
}

int GuiImageData::getHeight()
{
	return height;
}
