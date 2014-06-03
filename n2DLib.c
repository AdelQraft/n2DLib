#include "n2DLib.h"

// Double-buffering

int currentBuf = 0;
void *bufferBackup;
unsigned short *doubleBuf[2];

int initDoubleBuff()
{
	bufferBackup = (void*)SCREEN_BASE_ADDRESS;
	// Sets the monochrome screen to 16-bits
	if(!has_colors)
		*(int32_t*)0xC000001C = (*((int32_t*)0xC000001C) & ~0b1110) | 0b1000;
	doubleBuf[0] = (unsigned short*)malloc(BUFF_BYTES_SIZE);
	doubleBuf[1] = (unsigned short*)malloc(BUFF_BYTES_SIZE);
	
	return !doubleBuf[0] | !doubleBuf[1];
}

void switchBuffersRestore()
{
	memcpy(doubleBuf[currentBuf ^ 1], BUFF_BASE_ADDRESS, BUFF_BYTES_SIZE);
	*(unsigned short **)0xC0000010 = BUFF_BASE_ADDRESS;
	currentBuf ^= 1;
}

void switchBuffers()
{
	*(unsigned short **)0xC0000010 = BUFF_BASE_ADDRESS;
	currentBuf ^= 1;
}

void deinitDoubleBuff()
{
	// Sets the monochrome screen back to 4-bits
	if(!has_colors)
		*((int32_t*)0xC000001C) = (*((int32_t*)0xC000001C) & ~0b1110) | 0b0100;
	*(void**)0xC0000010 = bufferBackup;
	free(doubleBuf[0]);
	free(doubleBuf[1]);
}

// Maths
Fixed fixcos(Fixed angle)
{
	static Fixed cosLUT[] = { 256, 255, 255, 255, 254, 254, 253, 252, 251, 249, 248, 246, 244, 243, 241, 238, 236, 234, 231, 228, 225, 222, 219, 216, 212, 209, 205, 201, 197, 193, 189, 185, 181, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103, 97, 92, 86, 80, 74, 68, 62, 56, 49, 43, 37, 31, 25, 18, 12, 6, 0, -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92, -97, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176, -181, -185, -189, -193, -197, -201, -205, -209, -212, -216, -219, -222, -225, -228, -231, -234, -236, -238, -241, -243, -244, -246, -248, -249, -251, -252, -253, -254, -254, -255, -255, -255, -256, -255, -255, -255, -254, -254, -253, -252, -251, -249, -248, -246, -244, -243, -241, -238, -236, -234, -231, -228, -225, -222, -219, -216, -212, -209, -205, -201, -197, -193, -189, -185, -181, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103, -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12, -6, 0, 6, 12, 18, 25, 31, 37, 43, 49, 56, 62, 68, 74, 80, 86, 92, 97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234, 236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255 };
	return cosLUT[angle & 0xff];
}

void rotate(int x, int y, Fixed ca, Fixed sa, Rect* out)
{
	out->x = fixtoi(fixmul(itofix(x), ca) + fixmul(itofix(y), sa));
	out->y = fixtoi(fixmul(itofix(x), -sa) + fixmul(itofix(y), ca));
}

// Graphics

inline void clearBuffer()
{
	memset(BUFF_BASE_ADDRESS, 0, BUFF_BYTES_SIZE);
}

inline unsigned short getPixel(unsigned short *src, int x, int y)
{
	if(x >= 0 && x < src[0] && y >= 0 && y < src[1])
		return src[x + y * src[0] + 3];
	else
		return src[2];
}

inline void setPixel(int x, int y, unsigned short c)
{
	if(x >= 0 && x < 320 && y >= 0 && y < 240)
	{
		if(has_colors)
			*((unsigned short*)BUFF_BASE_ADDRESS + x + (y << 8) + (y << 6)) = c;
		else
		{
			c = ~c;
			*((unsigned short*)BUFF_BASE_ADDRESS + x + (y << 8) + (y << 6)) = ((c >> 11) + ((c & 0x07c0) >> 6) + (c & 0x1f)) & 0xffff;
		}
	}
}

inline void setPixelRGB(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	if(x >= 0 && x < 320 && y >= 0 && y < 240)
	{
		if(has_colors)
			*((unsigned short*)BUFF_BASE_ADDRESS + x + (y << 8) + (y << 6)) = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
		else
			*((unsigned short*)BUFF_BASE_ADDRESS + x + (y << 8) + (y << 6)) = ~(r + g + b) & 0xffff;
	}
}

void drawSprite(unsigned short *src, int _x, int _y)
{
	int x, y, w = src[0] + _x, h = src[1] + _y, c = 3;
	for(y = _y; y < h; y++)
	{
		for(x = _x; x < w; x++, c++)
		{
			if(src[c] != src[2])
				setPixel(x, y, src[c]);
		}
	}
}

void drawSpritePart(unsigned short *src, int _x, int _y, Rect part)
{
	unsigned short c;
	int x, y = _y, w = part.w + _x, h = part.h + _y, z = part.x, t = part.y;
	for(y = _y; y < h; y++, t++)
	{
		for(x = _x, z = part.x; x < w; x++, z++)
		{
			c = getPixel(src, z, t);
			if(c != src[2])
				setPixel(x, y, c);
		}
	}
}

void drawSpriteRotated(unsigned short* source, Rect sr, Fixed angle)
{
	Rect upleft, upright, downleft, downright;
	Rect fr;
	unsigned short currentPixel;
	Fixed dX = fixcos(angle), dY = fixsin(angle);
	
	rotate(-source[0] / 2, -source[1] / 2, dX, dY, &upleft);
	rotate(source[0] / 2, -source[1] / 2, dX, dY, &upright);
	rotate(-source[0] / 2, source[1] / 2, dX, dY, &downleft);
	rotate(source[0] / 2, source[1] / 2, dX, dY, &downright);
	
	fr.x = min(min(min(upleft.x, upright.x), downleft.x), downright.x) + sr.x;
	fr.y = min(min(min(upleft.y, upright.y), downleft.y), downright.y) + sr.y;
	fr.w = max(max(max(upleft.x, upright.x), downleft.x), downright.x) + sr.x;
	fr.h = max(max(max(upleft.y, upright.y), downleft.y), downright.y) + sr.y;
	
	Rect cp, lsp, cdrp;
	
	lsp.x = fixmul(itofix(fr.x - sr.x), dX) + fixmul(itofix(fr.y - sr.y), -dY);
	lsp.y = fixmul(itofix(fr.x - sr.x), dY) + fixmul(itofix(fr.y - sr.y), dX);
	
	for(cp.y = fr.y; cp.y < fr.h; cp.y++)
	{
		cdrp.x = lsp.x;
		cdrp.y = lsp.y;
		
		for(cp.x = fr.x; cp.x < fr.w; cp.x++)
		{
			if(cp.x >= 0 && cp.x < 320 && cp.y >= 0 && cp.y < 240)
			{
				if(abs(fixtoi(cdrp.x)) < source[0] / 2 && abs(fixtoi(cdrp.y)) < source[1] / 2)
				{
					currentPixel = getPixel(source, fixtoi(cdrp.x) + source[0] / 2, fixtoi(cdrp.y) + source[1] / 2);
					if(currentPixel != source[2])
					{
						setPixel(cp.x, cp.y, currentPixel);
					}
				}
			}
			cdrp.x += dX;
			cdrp.y += dY;
		}
		
		lsp.x -= dY;
		lsp.y += dX;
	}
}

/*	     *
 *  Geometry *
 *           */
 
void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
	int dx = abs(x2-x1);
	int dy = abs(y2-y1);
	int sx = (x1 < x2)?1:-1;
	int sy = (y1 < y2)?1:-1;
	int err = dx-dy;
	int e2;

	while (!(x1 == x2 && y1 == y2))
	{
		setPixelRGB(x1,y1,r,g,b);
		e2 = 2*err;
		if (e2 > -dy)
		{		 
			err = err - dy;
			x1 = x1 + sx;
		}
		if (e2 < dx)
		{		 
			err = err + dx;
			y1 = y1 + sy;
		}
	}
}

void drawPolygon(uint8_t r, uint8_t g, uint8_t b, int nombreDePoints, ...)
// r, g, b, <number of points you want (4 for a square, for instance, not 8 because of x and y...)>, <x1,y1,x2,y2...>
{
	// the number of arguments in the <...> must be even
	int i;
	int* pointsList = malloc(nombreDePoints*2*sizeof(int));
	
	va_list ap;
	int cur_arg = 1;

	va_start(ap, nombreDePoints);
	
	for (i = 0; i < nombreDePoints*2; i++)
	{
		cur_arg = va_arg(ap, int);
		*(pointsList + i) = cur_arg;
	}
	
	for (i = 0; i < nombreDePoints*2 - 2; i+=2)
	{
		drawLine(*(pointsList + i), *(pointsList + i + 1), *(pointsList + i + 2), *(pointsList + i + 3), r, g, b);
	}
	drawLine(*(pointsList + nombreDePoints*2 - 2), *(pointsList + nombreDePoints*2 - 1), *(pointsList), *(pointsList + 1), r, g, b);
	va_end(ap);
	free(pointsList);
}

void fillCircle(int x, int y, int radius, uint8_t r, uint8_t g, uint8_t b)
{
	int i,j;
	for(j=-radius; j<=radius; j++)
		for(i=-radius; i<=radius; i++)
			if(i*i+j*j <= radius*radius)
				setPixelRGB(x + i, y + j, r, g, b);
}

void fillEllipse(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
	int i,j;
	for(j=-h; j<=h; j++) 
		for(i=-w; i<=w; i++)
			if(i*i*h*h+j*j*w*w <= h*h*w*w)
				setPixelRGB(x + i, y + j, r, g, b);
}
