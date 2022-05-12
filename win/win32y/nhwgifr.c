#include "nhw.h"

typedef struct myTilesTag {
	LPBITMAPINFO	pbmp;	/* 256-color DIB */
	int	w;		/* image width */
	int	h;		/* image height */
	int	transparent;	/* -1:none */
} myTiles;

/*	GIF image format

	File Header
	Global Color Table	(if exist)	R,G,B,R,G,B,....
	Block
	Block
	 :

	    Image Block
		0x2C
		Image Header
		Local Color Table (if exist)
		Image Data
		    LZW compression size
		    Subblock
		    Subblock
		     :

	    Termination Block
		0x3B or 0x00

		Subblock
		    Length (0~0xFE)
		    Data   (Length bytes)
		    0x00

	    Control Extension Block (placed just before Image Block)
		0x21
		0xF9
		0x04
		Databit		[---dddut] ddd:How to dispose previous image / u:wait user input / t:use transparent color index
		WORD delaytime
		BYTE transparent color index
		0x00

	    Comment Extension Block
		0x21
		0xFE
		Subblock

	    Application Extension Block
		0x21
		0xFF
		0x0B
		id  BYTE[8]
		ver BYTE[3]
		Subblock

	    Plain Text Extension Block
		0x21
		0x01
		0x0C
		WORD[4]
		BYTE[4]
		Subblock
 */

/* -----------------------------------------------------------------
	global variables
   -----------------------------------------------------------------*/
HANDLE fstt;
int bitcnt;
int bitmask;
int bufindex;
int avbuf;
int avbit;
int bits;
int subblen;
static BYTE buf[256];

/* -----------------------------------------------------------------
	LZW
   -----------------------------------------------------------------*/
int LZWTbl[4097];

int dictindex;
int dictindex_bak;
int bitcnt_bak;
int bitmask_bak;

void initLZWtable(int minbitcnt) {
	int i;
	for (i=0; i<(1 << minbitcnt); i++) {
		LZWTbl[i] = -1;	/* itself */
	}
	LZWTbl[i++] = -2;	/* clear code */
	LZWTbl[i++] = -3;	/* end code */
	dictindex = i;
	dictindex_bak = i;
	for (; i<4097; i++) LZWTbl[i] = 0;
}
void entryLZWdict(int index, int len) {
	if (dictindex >= 4096) return;

	if (dictindex & ~bitmask) {
	    bitcnt++;
	    bitmask = (bitmask << 1) | 1;
	}

	LZWTbl[dictindex++] = index;
	LZWTbl[dictindex  ] = index + len - 1;
}

/* -----------------------------------------------------------------
	GIF
   -----------------------------------------------------------------*/
typedef struct {
	BYTE	id[6];		/* "GIF87a" "GIF89a" */
	WORD	width;		/* image width */
	WORD	height;		/* image height */
	BYTE	databit;	/* flags */
#define gColorTblMask		0x07
#define colorDepthMask		0x70
#define gColorTblExistMask	0x80
	BYTE	bgcolor;
	BYTE	aspectratio;	/* aspect ratio */
} FileHeader;

typedef struct {
	WORD	xoffset;
	WORD	yoffset;
	WORD	width;
	WORD	height;
	BYTE	databit;
#define	lColorTblMask		0x07
#define	interlacedMask		0x40
#define lColorTblExistMask	0x80
} ImgHeader;

BYTE gColorTable[256*3];

char *errmsg;

int readfile(HANDLE fh, LPVOID buf, DWORD size) {
	DWORD dummy;
	if (!ReadFile(fh, buf, size, &dummy, NULL)) {
	    errmsg = "File read error";
	    return 0;
	}
	if (size != dummy) {
	    errmsg = "Unexpected EOF while reading";
	    return 0;
	}
	return 1;
}

void initbits(HANDLE fh, int bcnt) {
	fstt = fh;
	bitcnt = bcnt;
	bitmask = (1 << bitcnt) - 1;
	bitcnt_bak = bitcnt;
	bitmask_bak = bitmask;
	avbuf = 0;
	avbit = 0;
}
int readbuf(void) {
	if (!avbuf) {
	    if (!readfile(fstt, buf, 1)) return -2;
	    if (!buf[0]) return -1;
	    avbuf = buf[0];
	    if (!readfile(fstt, buf, avbuf)) return -2;
	    bufindex = 0;
	}
	avbuf--;
	return (int)buf[bufindex++];
}
int readbits(void) {
	int d, newb;
	while (avbit < bitcnt) {
	    newb = readbuf();
	    if (newb < 0) return newb;
	    bits |= (newb << avbit);
	    avbit += 8;
	}
	d = bits & bitmask;
	bits >>= bitcnt;
	avbit -= bitcnt;
	return d;
}

int decompressLZW(BYTE *buf) {
	int i, j, k;
	int idx = 0;
	while (1) {
	    i = readbits();
	    if (i<0) return 0;
	    j = LZWTbl[i];
	    switch (j) {
		case -1: /* itself */
		    entryLZWdict(idx, 2);
		    buf[idx++] = i;
		    break;

		case -2: /* clear code */
		    dictindex = dictindex_bak;
		    bitcnt = bitcnt_bak;
		    bitmask = bitmask_bak;
		    break;
		case -3: /* end code */
		    return 1;

		default: /* code */
		    entryLZWdict(idx, LZWTbl[i+1] - j + 1 + 1);
		    for (k = j; k <= LZWTbl[i+1]; k++) {
			buf[idx++] = buf[k];
		    }
		    break;
	    }
	}
}

int interlace_info[10] = { 0,8, 4,8, 2,4, 1,2, 0,0 };
void copyBits(BYTE *dest, int dw, int dh, BYTE *src, int xstart, int ystart, int w, int h, int interlaced) {
	BYTE *ps, *pd;
	int x, y, i;
	int *yp, yd, ystep;

	ps = src;
	if (interlaced) {
	    yp = interlace_info;
	    y     = *yp++;
	    ystep = *yp++;
	} else {
	    y = 0;
	}
	for (i=0; i<h; i++) {
	    pd = &dest[dw * (dh-1 - (ystart + y)) + xstart];
	    for (x=0; x<w; x++) *pd++ = *ps++;
	    if (interlaced) {
		y += ystep;
		if (y >= h) {
		    y     = *yp++;
		    ystep = *yp++;
		}
	    } else {
		y++;
	    }
	}
}

LPBITMAPINFO loadBMPfile(HANDLE fh) {
	BITMAPFILEHEADER header;
	LPBITMAPINFOHEADER lpInfo;
	LPBYTE lpBuf;
	DWORD size, dummy;

	SetFilePointer(fh, 0, 0, FILE_BEGIN);
	ReadFile(fh, &header, sizeof(BITMAPFILEHEADER), &dummy, NULL);
	size = GetFileSize(fh, NULL) - sizeof(BITMAPFILEHEADER);
	lpBuf = GlobalAlloc(GPTR, size);
	ReadFile(fh, lpBuf, size, &dummy, NULL);

	lpInfo = (LPBITMAPINFOHEADER)(lpBuf);
	if (lpInfo->biSize != sizeof(BITMAPINFOHEADER) ||
	    lpInfo->biPlanes != 1 ||
	    lpInfo->biHeight < 0 ||
	    lpInfo->biCompression != BI_RGB) {
		errmsg = "Unsupported BMP format";
		return NULL;
	}
//	if (lpInfo->biBitCount != 8) {
//		errmsg = "Unsupported color depth (256 color only)";
//		return NULL;
//	}
	if (lpInfo->biClrUsed == 0) {
	    lpInfo->biClrUsed =
		(header.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER)) / 4;
	}

	return (LPBITMAPINFOHEADER)lpBuf;
}

int loadGIFfile(char *fn, myTiles *tilep) {
	HANDLE fh;
	FileHeader gifh;
	ImgHeader  imgh;
	int width, height;	/* BMP width/height (aligned) */
	int colorused;
	int interlaced;
	int loop;
	int i;

	LPBYTE lpBuf;
	LPBITMAPINFO lpInfo = NULL;
	LPRGBQUAD lpRGB;
	LPBYTE lpBit, lpWork;

	BYTE *p;
	BYTE b;

	char embuf[256];

	tilep->pbmp = NULL;
	tilep->transparent = -1;

	fh = CreateFile(fn, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
	    errmsg = "File cannot open";
	    goto xit;
	}

	/* read file header */
	if (!readfile(fh, &gifh, 13)) goto xit;
	if (gifh.id[0] != 'G' || gifh.id[1] != 'I' || gifh.id[2] != 'F' ||
	    gifh.id[3] != '8' || (gifh.id[4] != '7' && gifh.id[4] != '9') ||
	    gifh.id[5] != 'a') {
		if (gifh.id[0] == 'B' && gifh.id[1] == 'M') {
		    lpInfo = loadBMPfile(fh);
		    if (lpInfo == NULL) goto xit;
		    CloseHandle(fh);
		    tilep->pbmp = lpInfo;
		    tilep->w    = lpInfo->bmiHeader.biWidth;
		    tilep->h    = lpInfo->bmiHeader.biHeight;
		    return (lpInfo != NULL);
		}
		errmsg = "Unexpected GIF format";
		goto xit;
	}
	colorused = 1 << ((gifh.databit & gColorTblMask) + 1);
	width  = (gifh.width + 3) & 0xFFFFFFFC;
	height = gifh.height;
	lpBuf = GlobalAlloc(GPTR, sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD) +
				  width * height);
	if (lpBuf == NULL) {
	    errmsg = "Out of memory";
	    goto xit;
	}
	memset(lpBuf, 0, sizeof(BITMAPINFOHEADER));
	lpInfo = (LPBITMAPINFO)lpBuf;
	lpRGB  = (LPRGBQUAD)(lpBuf + sizeof(BITMAPINFOHEADER));
	lpBit  = (LPBYTE)(lpBuf + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
	lpInfo->bmiHeader.biSize	= sizeof(BITMAPINFOHEADER);
	lpInfo->bmiHeader.biWidth	= gifh.width;
	lpInfo->bmiHeader.biHeight	= gifh.height;
	lpInfo->bmiHeader.biPlanes	= 1;
	lpInfo->bmiHeader.biBitCount	= 8;
	lpInfo->bmiHeader.biCompression	= BI_RGB;
	lpInfo->bmiHeader.biClrUsed	= 256;

	/* read global color table (if exist) */
	if (gifh.databit & gColorTblExistMask) {
	    if (!readfile(fh, gColorTable, (1 << ((gifh.databit & gColorTblMask)+1))*3)) goto xit;
	    for (i=0, p=gColorTable; i<colorused; i++) {
		lpRGB[i].rgbRed   = *p++;
		lpRGB[i].rgbGreen = *p++;
		lpRGB[i].rgbBlue  = *p++;
	    }
	}

	/* fill bitmap with backgroung color */
	for (i=0, p=lpBit; i<width*height; i++) *p++ = gifh.bgcolor;

	/* read blocks */
	loop = 1;
	while (loop) {
	    if (!readfile(fh, &b, 1)) goto xit;
	    switch (b) {
		case 0x2C: /* Image Block */
		    if (!readfile(fh, &imgh, 9)) goto xit;
		    interlaced = !!(imgh.databit & interlacedMask);
		    if (imgh.databit & lColorTblExistMask) {
			if (!readfile(fh, gColorTable, (1 << ((imgh.databit & lColorTblMask)+1))*3)) goto xit;
			colorused = 1 << ((imgh.databit & lColorTblMask) + 1);
			for (i=0, p=gColorTable; i<colorused; i++) {
			    lpRGB[i].rgbRed   = *p++;
			    lpRGB[i].rgbGreen = *p++;
			    lpRGB[i].rgbBlue  = *p++;
			} /* no multiple color table support... just use the last one */
		    }
		    if (!readfile(fh, &b, 1)) goto xit;
		    initbits(fh, b+1);
		    initLZWtable(b);
		    lpWork = GlobalAlloc(GPTR, gifh.width * gifh.height);
		    if (lpWork == NULL) {
			errmsg = "Out of memory";
			goto xit;
		    }
		    if (!decompressLZW(lpWork)) goto xit;
		    copyBits(lpBit, width, height,
			     lpWork, imgh.xoffset, imgh.yoffset, imgh.width, imgh.height, interlaced);
		    GlobalFree(lpWork);
		    loop = 0;
		    break;

		case 0x3B: /* Termination Block */
		case 0x00: /* Termination Block */
		    loop = 0;
		    break;

		case 0x21: /* Extension Block */
		    if (!readfile(fh, &b, 1)) goto xit;
		    switch (b) {
			case 0xF9: {/* Control Extension Block */
			    BYTE ctrl[6];
			    if (!readfile(fh, ctrl, 6)) goto xit;
			    if (ctrl[0] != 0x04 || ctrl[5] != 0x00) goto subberr;
			    if (ctrl[1] & 0x01) {
				tilep->transparent = ctrl[4];
			    }
			}   break;

			case 0x01: /* Plain Text Extension Block */
			case 0xFE: /* Comment Extension Block */
			case 0xFF: /* Application Extension Block */
			    while (1) {
				if (!readfile(fh, &b, 1)) goto xit;	/* subblock length */
				if (b == 0x00) break;
				SetFilePointer(fh, (LONG)b, NULL, FILE_CURRENT); /* skip subblock */
			    }
			    break;

			default:
subberr:		    errmsg = "Unknown extension block";
			    goto xit;
		    }
		    break;

		default:
		    wsprintf(embuf, "Unknown block (%02X)", b);
		    errmsg = embuf;
		    goto xit;
	    }
	}

	/* done! */
	errmsg = 0;

xit:	if (errmsg) {
	    if (lpBuf) GlobalFree(lpBuf);
	    lpInfo = NULL;
	}
	CloseHandle(fh);

	tilep->pbmp = lpInfo;
	tilep->w    = gifh.width;
	tilep->h    = gifh.height;
	return (lpInfo != NULL);
}

int getPixelRGB(LPBITMAPINFO img, int x, int y, BYTE *r, BYTE *g, BYTE *b) {
	LPBITMAPINFOHEADER bi;
	LPRGBQUAD lpRGB;
	LPBYTE lpBit;
	int w0, c;
	if (img == NULL) return 0;
	bi = (LPBITMAPINFOHEADER)img;
	if (x < 0 || x >= bi->biWidth) return 0;
	if (y < 0 || y >= bi->biHeight) return 0;
	lpBit = (LPBYTE)(&img->bmiColors[0]);

	switch (bi->biBitCount) {
	    case 8:
		lpRGB = (LPRGBQUAD)lpBit;
		w0 = (bi->biWidth + 3) & (~3);
		c = bi->biClrUsed;
		if (!c) c = 256;
		lpBit += sizeof(RGBQUAD) * c;
		lpBit += (bi->biHeight - y - 1) * w0 + x;
		*b = lpRGB[*lpBit].rgbBlue;
		*g = lpRGB[*lpBit].rgbGreen;
		*r = lpRGB[*lpBit].rgbRed;
		break;
	    case 24:
		w0 = (bi->biWidth * 3 + 3) & (~3);
		lpBit += (bi->biHeight - y - 1) * w0 + x*3;
		*b = *lpBit++;
		*g = *lpBit++;
		*r = *lpBit;
		break;
	    case 32:
		w0 = bi->biWidth * 4;
		lpBit += (bi->biHeight - y - 1) * w0 + x*4;
		*b = *lpBit++;
		*g = *lpBit++;
		*r = *lpBit;
		break;
	    default:
		return 0;
	}
	return 1;
}

LPBYTE nhwLoadTiledFont(char *fn, int *fw, int *fh) {
	myTiles tinfo;
	LPBYTE lpTiledFont;
	BYTE r, g, b;
	int i, j, x, y, xx, yy;
	int dx, dy;

	if (!loadGIFfile(fn, &tinfo)) return NULL;
	lpTiledFont = GlobalAlloc(GPTR, tinfo.w * tinfo.h);
	if (lpTiledFont == NULL) goto xit;

	dx = tinfo.w >> 4;
	dy = tinfo.h >> 4;
	for (i=0, j=0; i<256; i++) {
	    yy = dy * (i >> 4);
	    for (y = yy; y < yy+dy; y++) {
		xx = dx * (i & 0x0F);
		for (x = xx; x < xx+dx; x++) {
		    if (getPixelRGB(tinfo.pbmp, x, y, &r, &g, &b)) {
			lpTiledFont[j++] = (g > 0x7F) ? 0xFF : 0x00;
		    }
		}
	    }
	}
xit:
	GlobalFree(tinfo.pbmp);
	*fw = dx;
	*fh = dy;
	return lpTiledFont;
}

