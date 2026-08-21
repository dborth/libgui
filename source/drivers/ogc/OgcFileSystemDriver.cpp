/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * OgcFileSystemDriver.cpp
 ***************************************************************************/
#include <fat.h>
#include "OgcFileSystemDriver.h"

void OgcFileSystemDriver::init()
{
	fatInitDefault();
}
void OgcFileSystemDriver::shutdown()
{

}
