#ifndef GME_GraphicsUtil_h
#define GME_GraphicsUtil_h

#include <map>
#include <vector>

typedef CTypedPtrList<CPtrList, Gdiplus::Font*>					GdipFontList;
typedef CTypedPtrMap<CMapPtrToPtr, void*, Gdiplus::Pen*>		GdipPenTable;
typedef CTypedPtrMap<CMapPtrToPtr, void*, Gdiplus::SolidBrush*>	GdipBrushTable;


class CArrowHead
{
public:
	CArrowHead(int dir, const std::vector<CPoint>& coords);
	virtual ~CArrowHead();
public:
	std::vector<CPoint> path;
	int dir;

public:
	CRect GetLineCoords(long index1, long index2, double angle, bool skew);
	virtual void Draw(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush, const CPoint& tip, bool bPen,
					  double angle, bool bold);
};

class CBulletArrowHead: public CArrowHead
{
public:
	CBulletArrowHead(int dir, const std::vector<CPoint>& coords);
	virtual ~CBulletArrowHead();

public:
	virtual void Draw(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush, const CPoint& tip, bool bPen,
					  double angle, bool bold);
};

class CGraphics
{
public:
	CGraphics();
	~CGraphics();

public:
	std::map<CString,Gdiplus::Pen*> m_mapGdipPens;
	GdipBrushTable gdipBrushes;
	CFont* normalFonts[GME_FONT_KIND_NUM];
	CFont* semiboldFonts[GME_FONT_KIND_NUM];
	CFont* boldFonts[GME_FONT_KIND_NUM];
	Gdiplus::Font* normalGdipFonts[GME_FONT_KIND_NUM];
	Gdiplus::Font* semiboldGdipFonts[GME_FONT_KIND_NUM];
	Gdiplus::Font* boldGdipFonts[GME_FONT_KIND_NUM];
	CArrowHead* arrows[GME_DIRECTION_NUM];
	CArrowHead* diamonds[GME_DIRECTION_NUM];
	CArrowHead* apexes[GME_DIRECTION_NUM];
	CArrowHead* bullets[GME_DIRECTION_NUM];
	CArrowHead* leftHalfArrows[GME_DIRECTION_NUM];
	CArrowHead* rightHalfArrows[GME_DIRECTION_NUM];

public:
	void	Initialize(void);
	void	Uninitialize(void);

	Gdiplus::Pen* GetGdipPen2(Gdiplus::Graphics* gdip, COLORREF color, GMEConnLineType lineType = GME_LINE_SOLID,
							  int width = 1);
	Gdiplus::Pen* GetGdipPen(Gdiplus::Graphics* gdip, COLORREF color, bool isPrinting, GMEConnLineType lineType = GME_LINE_SOLID,
							 int width = 1);
	Gdiplus::SolidBrush* GetGdipBrush(COLORREF color);
	Gdiplus::Font* GetGdipFont(GMEFontKind kind);
	Gdiplus::Font* GetGdipFont(int kindsize, bool bold = false, bool semibold = false);
	void DeleteBrushes(GdipBrushTable& gdipBrushTable);
	void CreateFonts(CFont** font, Gdiplus::Font** gdipFont, int boldness);
	void DrawGrid(Gdiplus::Graphics* gdip, int xSpace, int ySpace, int maxx, int maxy, COLORREF color = GME_GRID_COLOR);
	void DrawConnection(Gdiplus::Graphics* gdip, const CPointList& points, const std::vector<long>& customizedEdgeIndexes,
						COLORREF color, GMEConnLineType lineType, int srcEnd, int dstEnd,
						bool drawBullets, int width = 1);
	void DrawArrow(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush,
				   const CPoint& beforeLast, const CPoint& last, int iEnd, bool bold);
	void DrawGdipText(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font, COLORREF color,
					  int align);
	Gdiplus::RectF MeasureText2(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font);
	CSize MeasureText(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font);
};

extern CGraphics graphics;

#endif // whole file

