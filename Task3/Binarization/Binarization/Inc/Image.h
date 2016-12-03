#pragma once

#include <vector>

typedef std::vector< std::vector< BYTE > > TImage;

class CImage {
public:
	explicit CImage( const BitmapData& bmpData );
	// Возвращает уже БИНАРИЗОВАННОЕ изображение
	void GetData( BitmapData& data ) const;
	void NICKBinarization();

private:
	// Серое изображение
	TImage image;
	// Бинаризованного
	TImage binarizedImage;
	size_t height;
	size_t width;
	size_t zeroLevel;

	void fillUpperAndLowerEdges();
	void fillLeftRightEdges();
	BYTE getWindowthreshold( int i, int j ) const;
};
