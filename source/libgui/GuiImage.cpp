/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiImage.cpp
 ***************************************************************************/

#include "Gui.h"
#include <malloc.h>

GuiImage::GuiImage()
{
	image = nullptr;
	texture = nullptr;
	ownsTexture = false;
	textureDirty = false;
	width = 0;
	height = 0;
	imageangle = 0;
	tile = -1;
	stripe = 0;
	imgType = IMAGE::DATA;
}

GuiImage::GuiImage(GuiImageData * img)
{
	image = nullptr;
	texture = nullptr;
	ownsTexture = false;
	textureDirty = false;
	width = 0;
	height = 0;
	if(img)
	{
		image = img->getImage();
		texture = img->getTexture();
		width = img->getWidth();
		height = img->getHeight();
	}
	imageangle = 0;
	tile = -1;
	stripe = 0;
	imgType = IMAGE::DATA;
}

GuiImage::GuiImage(uint8_t * img, int w, int h)
{
	image = img;
	texture = nullptr;
	ownsTexture = true;
	textureDirty = true;
	width = w;
	height = h;
	imageangle = 0;
	tile = -1;
	stripe = 0;
	imgType = IMAGE::TEXTURE;
}

GuiImage::GuiImage(int w, int h, PixelColor c)
{
	image = (uint8_t *)malloc(w * h * 4);
	texture = nullptr;
	ownsTexture = true;
	textureDirty = true;
	width = w;
	height = h;
	imageangle = 0;
	tile = -1;
	stripe = 0;
	imgType = IMAGE::COLOR;

	if(!image)
		return;

	int x, y;

	for(y=0; y < h; ++y)
	{
		for(x=0; x < w; ++x)
		{
			this->setPixel(x, y, c);
		}
	}
}

GuiImage::~GuiImage()
{
	if(ownsTexture && texture)
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);

	if(imgType == IMAGE::COLOR && image)
		free(image);
}

uint8_t * GuiImage::getImage()
{
	return image;
}

void GuiImage::setImage(GuiImageData * img)
{
	if(ownsTexture && texture)
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);

	image = nullptr;
	texture = nullptr;
	ownsTexture = false;
	textureDirty = false;
	width = 0;
	height = 0;
	if(img)
	{
		image = img->getImage();
		texture = img->getTexture();
		width = img->getWidth();
		height = img->getHeight();
	}
	imgType = IMAGE::DATA;
}

void GuiImage::setImage(uint8_t * img, int w, int h)
{
	if(ownsTexture && texture)
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);

	image = img;
	texture = nullptr;
	ownsTexture = true;
	textureDirty = true;
	width = w;
	height = h;
	imgType = IMAGE::TEXTURE;
}

void GuiImage::setAngle(float a)
{
	imageangle = a;
}

void GuiImage::setTile(int t)
{
	tile = t;
}

PixelColor GuiImage::getPixel(int x, int y)
{
	if(!image || x < 0 || y < 0 || x >= this->getWidth() || y >= this->getHeight())
		return (PixelColor){0, 0, 0, 0};

	uint32_t offset = (y * this->getWidth() + x) * 4;
	PixelColor color;
	color.r = *(image+offset);
	color.g = *(image+offset+1);
	color.b = *(image+offset+2);
	color.a = *(image+offset+3);
	return color;
}

void GuiImage::setPixel(int x, int y, PixelColor color)
{
	if(!image || x < 0 || y < 0 || x >= this->getWidth() || y >= this->getHeight())
		return;

	uint32_t offset = (y * this->getWidth() + x) * 4;
	*(image+offset) = color.r;
	*(image+offset+1) = color.g;
	*(image+offset+2) = color.b;
	*(image+offset+3) = color.a;

	textureDirty = true;
}

void GuiImage::setStripe(int s)
{
	stripe = s;
}

void GuiImage::colorStripe(int shift)
{
	PixelColor color;
	int x, y=0;
	int alt = 0;
	
	int thisHeight =  this->getHeight();
	int thisWidth =  this->getWidth();

	for(; y < thisHeight; ++y)
	{
		if(y % 3 == 0)
			alt ^= 1;

		if(alt)
		{
			for(x=0; x < thisWidth; ++x)
			{
				color = getPixel(x, y);

				if(color.r < 255-shift)
					color.r += shift;
				else
					color.r = 255;
				if(color.g < 255-shift)
					color.g += shift;
				else
					color.g = 255;
				if(color.b < 255-shift)
					color.b += shift;
				else
					color.b = 255;

				color.a = 255;
				setPixel(x, y, color);
			}
		}
		else
		{
			for(x=0; x < thisWidth; ++x)
			{
				color = getPixel(x, y);

				if(color.r > shift)
					color.r -= shift;
				else
					color.r = 0;
				if(color.g > shift)
					color.g -= shift;
				else
					color.g = 0;
				if(color.b > shift)
					color.b -= shift;
				else
					color.b = 0;

				color.a = 255;
				setPixel(x, y, color);
			}
		}
	}

	refreshTexture();
}

void GuiImage::refreshTexture()
{
	if(imgType == IMAGE::DATA)
		return; // borrowed from GuiImageData -- not this object's to refresh

	if(!image)
		return;

	if(texture && !textureDirty)
		return;

	if(texture)
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);

	texture = platform->getVideo()->getImageRenderer()->createTexture(image, width, height);
	textureDirty = false;
}

/**
 * Draw the button on screen
 */
void GuiImage::draw()
{
	if(!image || !this->isVisible() || tile == 0)
		return;

	if(!texture)
		return;

	float currScaleX = this->getScaleX();
	float currScaleY = this->getScaleY();
	int currLeft = this->getLeft();
	int thisTop = this->getTop();

	if(tile > 0)
	{
		int alpha = this->getAlpha();
		for(int i=0; i<tile; ++i)
		{
			platform->getVideo()->getImageRenderer()->drawTexture(texture, currLeft+width*i, thisTop, width, height, imageangle, currScaleX, currScaleY, alpha);
		}
	}
	else
	{
		platform->getVideo()->getImageRenderer()->drawTexture(texture, currLeft, thisTop, width, height, imageangle, currScaleX, currScaleY, this->getAlpha());
	}

	if(stripe > 0)
	{
		int thisHeight = this->getHeight();
		int thisWidth = this->getWidth();
		for(int y=0; y < thisHeight; y+=6)
			platform->getVideo()->getImageRenderer()->drawRectangle(currLeft,thisTop+y,thisWidth,3,(PixelColor){0, 0, 0, (uint8_t)stripe},1);
	}
	this->updateEffects();
}
