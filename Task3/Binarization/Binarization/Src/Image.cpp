#include <Common.h>
#pragma hdrstop

#include <Image.h>

CImage::CImage( const BitmapData& bmpData )
{
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	width = bmpData.Width;
	height = bmpData.Height;
	image.resize( height );
	int baseAdr = 0;
	for( size_t y = 0; y < height; y++ ) {
		image[y].resize( width );
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < width; x++ ) {
			BYTE B = pBuffer[pixelAdr]; // blue
			BYTE G = pBuffer[pixelAdr + 1]; // green
			BYTE R = pBuffer[pixelAdr + 2]; // red
			image[y][x] = ( int ) ( 0.2125 * R + 0.7154 * G + 0.0721 * B );
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

void CImage::GetData( BitmapData& bmpData ) const
{
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	size_t width = bmpData.Width;
	size_t height = bmpData.Height;
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	int baseAdr = 0;
	for( size_t y = 0; y < height; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < width; x++ ) {
			pBuffer[pixelAdr] = image[y][x];
			pBuffer[pixelAdr + 1] = image[y][x];
			pBuffer[pixelAdr + 2] = image[y][x];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

void CImage::NICKBinarization( const size_t windowSize )
{
	// todo:
}