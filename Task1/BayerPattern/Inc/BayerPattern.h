#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <time.h>
#include <iostream>
#include <vector>

typedef std::vector< std::vector< COLORREF > > TImage;

using namespace Gdiplus;
using std::cout;
using std::endl;

class CBayerPattern {
public:
	explicit CBayerPattern( const BitmapData& bmpData );

	// Pixel Grouping ��������
	void Process();

	void GetData( BitmapData& bmpData ) const;

private:
	// ����������� ������� height + 4 X width + 4
	// ��� ��� �� ���� ������ ��� ����� � �������� ���������.
	// �� ���� ������ �� 2 ������ ������� ������ �����������.
	TImage image;
	size_t height;
	size_t width;

	void restoreGreen();
	void restoreBlueRed();
	bool isBlue( const size_t x, const size_t y ) const;
	bool isGreen( const size_t x, const size_t y ) const;
	bool isRed( const size_t x, const size_t y ) const;
};