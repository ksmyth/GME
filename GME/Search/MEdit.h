#pragma once


// CMEdit

class CMEdit : public CEdit
{
	DECLARE_DYNAMIC(CMEdit)

public:
	CMEdit();
	virtual ~CMEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};
