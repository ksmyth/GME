#include "stdafx.h"

#include "GMEstd.h"
#include "GraphicsUtil.h"
#define _USE_MATH_DEFINES
#include <math.h>


static bool fontBoldness[GME_FONT_KIND_NUM]		= { false, false, false };
static bool fontSemiboldness[GME_FONT_KIND_NUM]	= { true, true, false };
static int  fontSizes[GME_FONT_KIND_NUM]		= { 16, 15, 12 };


//////////////////////////////////// CArrowHead //////////////////////////////

CArrowHead::CArrowHead(int dir, const std::vector<CPoint>& coords)
{
	this->dir = dir;
	ASSERT(coords.size() > 0);
	std::vector<CPoint>::const_iterator ii = coords.begin();
	while(ii != coords.end()) {
		CPoint coord;
		switch(dir) {
		case GME_RIGHT_DIRECTION:
			coord.x = (*ii).x;
			coord.y = (*ii).y;
			break;
		case GME_LEFT_DIRECTION:
			coord.x = -(*ii).x;
			coord.y = -(*ii).y;
			break;
		case GME_UP_DIRECTION:
			coord.x = (*ii).y;
			coord.y = -(*ii).x;
			break;
		case GME_DOWN_DIRECTION:
			coord.x = -(*ii).y;
			coord.y = (*ii).x;
			break;
		default:
			ASSERT(false);
			break;
		}
		path.push_back(coord);
		++ii;
	}
}

CArrowHead::~CArrowHead()
{
}

CRect CArrowHead::GetLineCoords(long index1, long index2, double angle, bool skew)
{
	CRect line;
	if (!skew) {
		line.left	= path[index1].x;
		line.top	= path[index1].y;
		line.right	= path[index2].x;
		line.bottom	= path[index2].y;
	} else {
		line.left	= (int)(cos(angle) * path[index1].x - sin(angle) * path[index1].y + 0.5);
		line.top	= (int)(sin(angle) * path[index1].x + cos(angle) * path[index1].y + 0.5);
		line.right	= (int)(cos(angle) * path[index2].x - sin(angle) * path[index2].y + 0.5);
		line.bottom	= (int)(sin(angle) * path[index2].x + cos(angle) * path[index2].y + 0.5);
	}
	return line;
}

void CArrowHead::Draw(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush, const CPoint& tip, bool bPen,
					  double angle, bool bold)
{
	bool skew = (abs(angle) > 1.0e-6);
	Gdiplus::GraphicsPath arrowHeadPath;
	int segments = path.size();
	long lastx;
	long lasty;
	for (int i = 1; i < segments; i++) {
		CRect line = GetLineCoords(i - 1, i, angle, skew);
		arrowHeadPath.AddLine(line.left + tip.x, line.top + tip.y, line.right + tip.x, line.bottom + tip.y);
		lastx = line.left + tip.x;
		lasty = line.top + tip.y;
	}

	if (bold && !bPen)
	{
		if (dir == GME_LEFT_DIRECTION || dir == GME_RIGHT_DIRECTION)
		{
			Gdiplus::Matrix matrix(1.0f, 0.0f, 0.0f, 1.7f, 0, (Gdiplus::REAL)-lasty * (1.7 - 1));
			arrowHeadPath.Transform(&matrix);
		}
		else
		{
			Gdiplus::Matrix matrix(1.7f, 0.0f, 0.0f, 1.0f, (Gdiplus::REAL)-lastx * (1.7 - 1), 0);
			arrowHeadPath.Transform(&matrix);
		}
	}
	gdip->FillPath(brush, &arrowHeadPath);

	if (bPen) {
		std::vector<Gdiplus::Point> points;
		points.reserve(path.size() + 2);
		for (int i = 0; i < path.size(); i++) {
			points.push_back(Gdiplus::Point((int)path[i].x + tip.x, (int)path[i].y + tip.y));
		}
		CRect line = GetLineCoords(segments - 1, 0, angle, skew);
		points.push_back(Gdiplus::Point((int)path[0].x + tip.x, (int)path[0].y + tip.y));
		points.push_back(Gdiplus::Point((int)path[1].x + tip.x, (int)path[1].y + tip.y)); // <- extra point gives nice sharp corner, instead of butt-end
		if (bold)
		{
			pen = pen->Clone();
			Gdiplus::Matrix mat(3.0, 0.0, 0.0, 3.0, 0, 0);
			pen->MultiplyTransform(&mat); // FIXME only shows up at higher zoom for some reason
		}
		gdip->DrawLines(pen, &points[0], points.size());
		if (bold)
		{
			delete pen;
		}
	}

}


//////////////////////////////////// CBulletArrowHead //////////////////////////////

CBulletArrowHead::CBulletArrowHead(int dir, const std::vector<CPoint>& coords):
	CArrowHead(dir, coords)
{
}

CBulletArrowHead::~CBulletArrowHead()
{
}

void CBulletArrowHead::Draw(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush, const CPoint& tip, bool bPen,
							double angle, bool bold)
{
	ASSERT(path.size() == 2);
	int ox = 0;
	int oy = 0;
	if (abs(angle) < 1.0e-6)
	{
		ox = path[0].x;
		oy = path[0].y;
	}
	else	// skew
	{
		ox = (int)(cos(angle) * path[0].x - sin(angle) * path[0].y + 0.5);
		oy = (int)(sin(angle) * path[0].x + cos(angle) * path[0].y + 0.5);
	}
	int a = abs(path[1].x);
	int b = abs(path[1].y);
	Gdiplus::GraphicsPath* bulletPath = new Gdiplus::GraphicsPath();
	bulletPath->AddEllipse(ox - a + tip.x, oy - b + tip.y, 2 * a, 2 * b);
	gdip->FillPath(brush, bulletPath);
	gdip->DrawPath(pen, bulletPath);
	delete bulletPath;
}


//////////////////////////////////// CGraphics //////////////////////////////

CGraphics::CGraphics()
{
}

CGraphics::~CGraphics()
{
}

void CGraphics::Initialize(void)
{
	CreateFonts(normalFonts,	normalGdipFonts,	FW_LIGHT);
	CreateFonts(semiboldFonts,	semiboldGdipFonts,	FW_NORMAL);
	CreateFonts(boldFonts,		boldGdipFonts,		FW_SEMIBOLD);

	std::vector<CPoint> arrowCoords;
	arrowCoords.push_back(CPoint(-5, 0));
	arrowCoords.push_back(CPoint(-7, -3));
	arrowCoords.push_back(CPoint(-1, 0));
	arrowCoords.push_back(CPoint(-7, 3));
	std::vector<CPoint> leftHalfCoords;
	leftHalfCoords.push_back(CPoint(-1, 0));
	leftHalfCoords.push_back(CPoint(-10, -5));
	std::vector<CPoint> rightHalfCoords;
	rightHalfCoords.push_back(CPoint(-1, 0));
	rightHalfCoords.push_back(CPoint(-10, 5));
	std::vector<CPoint> diamondCoords;
	diamondCoords.push_back(CPoint(-10, 0));
	diamondCoords.push_back(CPoint(-5, -3));
	diamondCoords.push_back(CPoint(-0, 0));
	diamondCoords.push_back(CPoint(-5, 3));
	std::vector<CPoint> apexCoords;
	apexCoords.push_back(CPoint(-9, -5));
	apexCoords.push_back(CPoint(-9, 5));
	apexCoords.push_back(CPoint(-1, 0));
	std::vector<CPoint> bulletCoords;		// Special:
	bulletCoords.push_back(CPoint(-4, 0));	// ellipse origo
	bulletCoords.push_back(CPoint(3, 3));	// a and b radius of ellipse

	for(int i = 0; i < GME_DIRECTION_NUM; i++) {
		arrows[i]			= new CArrowHead( i, arrowCoords );
		diamonds[i]			= new CArrowHead( i, diamondCoords );
		apexes[i]			= new CArrowHead( i, apexCoords );
		bullets[i]			= new CBulletArrowHead( i, bulletCoords );
		leftHalfArrows[i]	= new CArrowHead( i, leftHalfCoords );
		rightHalfArrows[i]	= new CArrowHead( i, rightHalfCoords );
	}
}

void CGraphics::Uninitialize(void)
{
	for (std::map<CString,Gdiplus::Pen*>::iterator it = m_mapGdipPens.begin() ; it != m_mapGdipPens.end() ; ++it)
		delete it->second;
	DeleteBrushes(gdipBrushes);

	for (int i = 0; i < GME_FONT_KIND_NUM; i++) {
		delete normalFonts[i];
		delete semiboldFonts[i];
		delete boldFonts[i];
		delete normalGdipFonts[i];
		delete semiboldGdipFonts[i];
		delete boldGdipFonts[i];
	}

	for (int i = 0; i < GME_DIRECTION_NUM; i++) {
		delete arrows[i];
		delete diamonds[i];
		delete apexes[i];
		delete bullets[i];
		delete leftHalfArrows[i];
		delete rightHalfArrows[i];
	}
}

void CGraphics::DeleteBrushes(GdipBrushTable& gdipBrushTable)
{
	POSITION pos = gdipBrushTable.GetStartPosition();
	void* pt;
	Gdiplus::SolidBrush* gdipBrush;
	while(pos) {
		gdipBrushTable.GetNextAssoc(pos, pt, gdipBrush);
		delete gdipBrush;
	}
}

void CGraphics::CreateFonts(CFont** font, Gdiplus::Font** gdipFont, int boldness)
{
	for (int i = 0; i < GME_FONT_KIND_NUM; i++) {
		font[i] = new CFont();
		font[i]->CreateFont(fontSizes[i],0,0,0,boldness,0,0,0,ANSI_CHARSET,
							OUT_DEVICE_PRECIS,CLIP_DEFAULT_PRECIS,
							PROOF_QUALITY,FF_SWISS,_T("Arial"));

		CDC dc;
		dc.CreateCompatibleDC(NULL);
		LOGFONT logFont;
		font[i]->GetLogFont(&logFont);
		gdipFont[i] = new Gdiplus::Font( dc.m_hDC, &logFont );
	}
}

Gdiplus::Font* CGraphics::GetGdipFont(GMEFontKind kind)
{
	return GetGdipFont(kind, fontBoldness[kind], fontSemiboldness[kind]);
}

Gdiplus::Font* CGraphics::GetGdipFont(int kindsize, bool bold, bool semibold)
{
	return bold ? boldGdipFonts[kindsize] : (semibold ? semiboldGdipFonts[kindsize] : normalGdipFonts[kindsize]);
}

Gdiplus::Pen* CGraphics::GetGdipPen2(Gdiplus::Graphics* gdip, COLORREF color, GMEConnLineType lineType,
									 int width)
{
	// TODO
//	HDC hDC = gdip->GetHDC();
//	bool isPrinting = GetDeviceCaps(hDC, TECHNOLOGY) == DT_RASPRINTER;
//	gdip->ReleaseHDC(hDC);
	return GetGdipPen(gdip, color, /*isPrinting*/ false, lineType, width);
}

Gdiplus::Pen* CGraphics::GetGdipPen(Gdiplus::Graphics* gdip, COLORREF color, bool isPrinting, GMEConnLineType lineType,
									int width)
{
	CString chBuffer;
	chBuffer.Format(_T("%x-%d-%d-%ld"), color, isPrinting, lineType, width);
	std::map<CString,Gdiplus::Pen*>::iterator it = m_mapGdipPens.find(chBuffer);
	if (it != m_mapGdipPens.end())
		return it->second;

	Gdiplus::Color gdipColor(GetRValue(color), GetGValue(color), GetBValue(color));
	float fWidth = static_cast<float> (isPrinting ? 1 : width);
	Gdiplus::Pen* pen = new Gdiplus::Pen(gdipColor, fWidth);
	if (lineType == GME_LINE_DASH) {
		Gdiplus::REAL dashPatternVals[2] = {
			5.0f,	// dash length 5
			2.0f	// space length 2
		};
		pen->SetDashPattern(dashPatternVals, 2);
		pen->SetDashStyle(Gdiplus::DashStyleCustom);
		pen->SetDashCap(Gdiplus::DashCapRound);
	} else if (lineType == GME_LINE_CUSTOMIZED) {
		pen->SetDashStyle(Gdiplus::DashStyleDashDotDot);
		pen->SetDashCap(Gdiplus::DashCapRound);
//	} else {
//		pen->SetDashStyle(Gdiplus::DashStyleSolid);
	}

	m_mapGdipPens.insert(std::map<CString,Gdiplus::Pen*>::value_type(chBuffer, pen));
	return pen;
}

Gdiplus::SolidBrush* CGraphics::GetGdipBrush(COLORREF color)
{
	Gdiplus::SolidBrush* brush = NULL;
	if(!(gdipBrushes.Lookup((void*)color, brush))) {
		brush = new Gdiplus::SolidBrush(Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color)));
		gdipBrushes.SetAt((void*)color, brush);
	}
	ASSERT(brush);
	return brush;
}

void CGraphics::DrawGrid(Gdiplus::Graphics* gdip, int xSpace, int ySpace, int maxx, int maxy, COLORREF color)
{
	Gdiplus::Pen* pen = GetGdipPen2(gdip, color);
	for(int x = 0 ; x < maxx; x += xSpace)
		gdip->DrawLine(pen, x, 0, x, maxy);
	for(int y = 0; y < maxy; y += ySpace)
		gdip->DrawLine(pen, 0, y, maxx, y);
}

typedef std::pair<long,long> Long_Pair;

void CGraphics::DrawConnection(Gdiplus::Graphics* gdip, const CPointList& points, const std::vector<long>& customizedEdgeIndexes,
							   COLORREF color, GMEConnLineType lineType, int srcEnd, int dstEnd,
							   bool drawBullets, int width)
{
	if (points.GetCount() == 0)
		return;

	long numEdges = points.GetCount() - 1;
	if (numEdges < 1) {
		ASSERT(false);
		return;
	}

	// TODO?
//	HDC hDC = gdip->GetHDC();
	bool isPrinting = false;	//GetDeviceCaps(hDC, TECHNOLOGY) == DT_RASPRINTER;
//	gdip->ReleaseHDC(hDC);
//	if (isPrinting)
//		color = GME_BLACK_COLOR;

	Gdiplus::Brush* brush = GetGdipBrush(color);
	Gdiplus::Brush* bulletBrush = brush;
	long bulletOffset = (width + 4) / 2;
	long bulletRadius = width + 4;

	if (customizedEdgeIndexes.size() > 0)	{
		std::map<long,long> customizedIndexes; // FIXME if customizedEdgeIndexes is sorted, use binary search to avoid allocations
		// convert array to a map for easier lookup
		std::vector<long>::const_iterator ii = customizedEdgeIndexes.begin();
		while (ii != customizedEdgeIndexes.end())
		{
			customizedIndexes.insert(Long_Pair(*ii, 0));
			++ii;
		}
		std::map<long,long>::const_iterator indIter;

		CPoint last;
		POSITION pos = points.GetHeadPosition();
		if (pos) {
			CPoint pt = points.GetNext(pos);
			last = pt;
			long currEdgeIndex = 0;
			while (pos) {
				pt = points.GetNext(pos);
				Gdiplus::Pen* pen;
				indIter = customizedIndexes.find(currEdgeIndex);
				if (indIter != customizedIndexes.end()) {
					pen = GetGdipPen(gdip, color, isPrinting, GME_LINE_CUSTOMIZED, width);
				} else {
					pen = GetGdipPen(gdip, color, isPrinting, lineType, width);
				}
				gdip->DrawLine(pen, last.x, last.y, pt.x, pt.y);
				if (drawBullets && currEdgeIndex < numEdges - 1) {
					gdip->FillEllipse(bulletBrush, pt.x - bulletOffset, pt.y - bulletOffset, bulletRadius, bulletRadius);
				}
				last = pt;
				currEdgeIndex++;
			}
		}
	}
	else {
		// Same pen for all edges. Use DrawLines.
		using namespace Gdiplus;
		std::vector<Point> gpoints;
		gpoints.reserve(points.GetSize());
		POSITION pos = points.GetHeadPosition();
		while (pos) {
			CPoint pt = points.GetNext(pos);
			gpoints.push_back(Point(pt.x, pt.y));
		}
		Pen* pen = GetGdipPen(gdip, color, isPrinting, lineType, width);
		gdip->DrawLines(pen, &gpoints[0], gpoints.size()); // n.b. this is different than a sequence of DrawLine()s (GME-419)
	}

	Gdiplus::Pen* headpen = GetGdipPen(gdip, color, isPrinting);

	{
	POSITION pos2 = points.GetHeadPosition();
	CPoint first = points.GetNext(pos2);
	CPoint second = points.GetNext(pos2);

	Gdiplus::Brush* headBrush = NULL;
	headBrush = GetGdipBrush((srcEnd == GME_EMPTYDIAMOND_END ||
							  srcEnd == GME_EMPTYAPEX_END ||
							  srcEnd == GME_EMPTYBULLET_END) ? GME_WHITE_COLOR : color);

	DrawArrow(gdip, headpen, headBrush, second, first, srcEnd, width > 2);
	}

	{
	POSITION pos = points.GetTailPosition();
	CPoint last = points.GetPrev(pos);
	CPoint beforeLast = points.GetPrev(pos);

	Gdiplus::Brush* headBrush = NULL;
	headBrush = GetGdipBrush((dstEnd == GME_EMPTYDIAMOND_END ||
							  dstEnd == GME_EMPTYAPEX_END ||
							  dstEnd == GME_EMPTYBULLET_END) ? GME_WHITE_COLOR : color);

	DrawArrow(gdip, headpen, headBrush, beforeLast, last, dstEnd, width > 2);
	}
}

void CGraphics::DrawArrow(Gdiplus::Graphics* gdip, Gdiplus::Pen* pen, Gdiplus::Brush* brush,
						  const CPoint& beforeLast, const CPoint& last, int iEnd, bool bold)
{
	if (iEnd == GME_BUTT_END) {
		return;
	}
	int dir = 0;
	double alpha = 0.0;
	bool skew = false;
	if (beforeLast.x == last.x)
	{
		if (beforeLast.y > last.y)
		{
			dir = GME_UP_DIRECTION;
		}
		else
		{
			dir = GME_DOWN_DIRECTION;
		}
	}
	else if (beforeLast.y == last.y)
	{
		if (beforeLast.x > last.x)
		{
			dir = GME_LEFT_DIRECTION;
		}
		else
		{
			dir = GME_RIGHT_DIRECTION;
		}
	}
	else	// skew
	{
		skew = true;
		dir = GME_RIGHT_DIRECTION;
		alpha = atan2((double)last.y - beforeLast.y, (double)last.x - beforeLast.x);
	}

	switch (iEnd) {
		case GME_ARROW_END :
			arrows[dir]->Draw(gdip, pen, brush, last, false, alpha, bold);
			break;
		case GME_DIAMOND_END :
		case GME_EMPTYDIAMOND_END :
			diamonds[dir]->Draw(gdip, pen, brush, last, iEnd == GME_EMPTYDIAMOND_END, alpha, bold);
			break;
		case GME_APEX_END :
		case GME_EMPTYAPEX_END :
			apexes[dir]->Draw(gdip, pen, brush, last, iEnd == GME_EMPTYAPEX_END, alpha, bold);
			break;
		case GME_BULLET_END :
		case GME_EMPTYBULLET_END :
			bullets[dir]->Draw(gdip, pen, brush, last, iEnd == GME_EMPTYBULLET_END, alpha, bold);
			break;
		case GME_HALFARROWLEFT_END :
			leftHalfArrows[dir]->Draw(gdip, pen, brush, last, true, alpha, bold);
			break;
		case GME_HALFARROWRIGHT_END :
			rightHalfArrows[dir]->Draw(gdip, pen, brush, last, true, alpha, bold);
			break;
	}
}

void CGraphics::DrawGdipText(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font, COLORREF color, int align)
{
	if (font == NULL)
		return;

	Gdiplus::StringFormat format;
	Gdiplus::StringAlignment horizontalAlignment;
	if ((align & TA_CENTER) == TA_CENTER) {
		horizontalAlignment = Gdiplus::StringAlignmentCenter;
	} else if ((align & TA_RIGHT) == TA_RIGHT) {
		horizontalAlignment = Gdiplus::StringAlignmentFar;
	} else {	// TA_LEFT
		horizontalAlignment = Gdiplus::StringAlignmentNear;
	}
	format.SetAlignment(horizontalAlignment);
	Gdiplus::StringAlignment verticalAlignment;
	if ((align & TA_BASELINE) == TA_BASELINE) {
		verticalAlignment = Gdiplus::StringAlignmentCenter;
	} else if ((align & TA_BOTTOM) == TA_BOTTOM) {
		verticalAlignment = Gdiplus::StringAlignmentFar;
	} else {	// TA_TOP
		verticalAlignment = Gdiplus::StringAlignmentNear;
	}
	format.SetLineAlignment(verticalAlignment);
	format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap);
	format.SetTrimming(Gdiplus::StringTrimmingEllipsisPath);

	// TODO
//	HDC hDC = gdip->GetHDC();
//	COLORREF textColor = GetDeviceCaps(hDC, TECHNOLOGY) == DT_RASPRINTER ? GME_BLACK_COLOR : color;
//	gdip->ReleaseHDC(hDC);
	COLORREF textColor = color;

	Gdiplus::SolidBrush textBrush(Gdiplus::Color(GetRValue(textColor),
												 GetGValue(textColor),
												 GetBValue(textColor)));

	Gdiplus::PointF pointF(static_cast<float> (pt.x),
						   static_cast<float> (pt.y));

	gdip->DrawString(txt, txt.GetLength(), font, pointF, &format, &textBrush);
}

Gdiplus::RectF CGraphics::MeasureText2(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font)
{
	Gdiplus::PointF pointF(static_cast<float> (pt.x),
						   static_cast<float> (pt.y));

	Gdiplus::RectF rectF;
	gdip->MeasureString(txt, txt.GetLength(), font, pointF, &rectF);
	return rectF;
}

CSize CGraphics::MeasureText(Gdiplus::Graphics* gdip, const CString& txt, const CPoint& pt, Gdiplus::Font* font)
{
	Gdiplus::RectF rectF = MeasureText2(gdip, txt, pt, font);
	Gdiplus::SizeF sizeF;
	rectF.GetSize(&sizeF);
	CSize size(static_cast<long> (sizeF.Width), static_cast<long> (sizeF.Height));
	return size;
}
