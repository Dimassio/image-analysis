#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <time.h>
#include <iostream>
#include <vector>

typedef std::vector< std::vector< COLORREF > > TImage;

using namespace Gdiplus;

class CBayerPattern {
public:
	explicit CBayerPattern( const BitmapData& bmpData );

	// Pixel Grouping אכדמנטעל
	void Process();

	void GetData( BitmapData& bmpData ) const;

private:
	TImage image;
	size_t height;
	size_t width;

	void restoreGreen();
	void restoreBlueRed();
	bool isBlue( const size_t x, const size_t y ) const;
	bool isGreen( const size_t x, const size_t y ) const;
	bool isRed( const size_t x, const size_t y ) const;
};