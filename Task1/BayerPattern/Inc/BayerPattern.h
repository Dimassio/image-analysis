#pragma once

#include <Common.h>
#include <windows.h>
#include <gdiplus.h>
#include <time.h>
#include <vector>

// Если значение по умолчанию для рамки не задано, то боковые строчки будут экстраполированы на 2 пикселя во все стороны,
// если задано - то def - значение этих пикселей
#define FRAME_VALUE(R, G, B, def) def == -1 ? RGB(R, G, B) : def;

typedef std::vector< std::vector< COLORREF > > TImage;

using namespace Gdiplus;

// Класс обертка - представляет собой изображения в виде 2мерной матрицы, в ячейках которой
// лежат цвета в формате COLORREF. При создании объекта, матрица экстраполируется на 2 пикселя 
// с каждой стороны. По умолчанию эти пиксели просто дублируют соответсвтующие пиксели в строчках(столбцах)
class CBayerPattern {
public:
	explicit CBayerPattern( const BitmapData& bmpData, int defFrameValue = -1 );

	// Pixel Grouping алгоритм
	void Process();

	void GetData( BitmapData& bmpData ) const;

private:
	// Изображение размера height + 4 X width + 4
	// так как не хочу думать что будет с крайними пикселями.
	// Со всех сторон на 2 строки пиксели просто дублируются.
	TImage image;
	size_t height;
	size_t width;

	void restoreGreen();
	void restoreBlueRed();
	void computeRedBlueAtGreen();
	void computeBlueAtRed();
	void computeRedAtBlue();
	bool isBlueOnly( const size_t x, const size_t y ) const;
	bool isGreenOnly( const size_t x, const size_t y ) const;
	bool isRedOnly( const size_t x, const size_t y ) const;
	int hueTransit( int l1, int l2, int l3, int v1, int v3 ) const;
	void gammaCorrection( double gamma );
	int linAndGamma( int value );
};