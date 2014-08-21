// 28 july 2014

#include "winapi_windows.h"
#include "_cgo_export.h"

// provided for cgo's benefit
LPWSTR xWC_LISTVIEW = WC_LISTVIEW;

static void handle(HWND hwnd, WPARAM wParam, LPARAM lParam, void (*handler)(void *, int, int), void *data)
{
	LVHITTESTINFO ht;

	// TODO use wParam
	ZeroMemory(&ht, sizeof (LVHITTESTINFO));
	ht.pt.x = GET_X_LPARAM(lParam);
	ht.pt.y = GET_Y_LPARAM(lParam);
	if (SendMessageW(hwnd, LVM_SUBITEMHITTEST, 0, (LPARAM) (&ht)) == (LRESULT) -1) {
		(*handler)(data, -1, -1);
		return;		// no item
	}
	if (ht.flags != LVHT_ONITEMSTATEICON) {
		(*handler)(data, -1, -1);
		return;		// not on a checkbox
	}
	(*handler)(data, ht.iItem, ht.iSubItem);
}

static struct {int code; char *name;} lvnnames[] = {
{ LVN_ITEMCHANGING, "LVN_ITEMCHANGING" },
{ LVN_ITEMCHANGED, "LVN_ITEMCHANGED" },
{ LVN_INSERTITEM, "LVN_INSERTITEM" },
{ LVN_DELETEITEM, "LVN_DELETEITEM" },
{ LVN_DELETEALLITEMS, "LVN_DELETEALLITEMS" },
{ LVN_BEGINLABELEDITA, "LVN_BEGINLABELEDITA" },
{ LVN_BEGINLABELEDITW, "LVN_BEGINLABELEDITW" },
{ LVN_ENDLABELEDITA, "LVN_ENDLABELEDITA" },
{ LVN_ENDLABELEDITW, "LVN_ENDLABELEDITW" },
{ LVN_COLUMNCLICK, "LVN_COLUMNCLICK" },
{ LVN_BEGINDRAG, "LVN_BEGINDRAG" },
{ LVN_BEGINRDRAG, "LVN_BEGINRDRAG" },
//{ LVN_ODCACHEHINT, "LVN_ODCACHEHINT" },
{ LVN_ODFINDITEMA, "LVN_ODFINDITEMA" },
{ LVN_ODFINDITEMW, "LVN_ODFINDITEMW" },
{ LVN_ITEMACTIVATE, "LVN_ITEMACTIVATE" },
{ LVN_ODSTATECHANGED, "LVN_ODSTATECHANGED" },
{ LVN_SETDISPINFOA, "LVN_SETDISPINFOA" },
{ LVN_SETDISPINFOW, "LVN_SETDISPINFOW" },
//{ LVN_KEYDOWN, "LVN_KEYDOWN" },
{ LVN_MARQUEEBEGIN, "LVN_MARQUEEBEGIN" },
{ LVN_GETINFOTIPA, "LVN_GETINFOTIPA" },
{ LVN_GETINFOTIPW, "LVN_GETINFOTIPW" },
{ LVN_BEGINSCROLL, "LVN_BEGINSCROLL" },
{ LVN_ENDSCROLL, "LVN_ENDSCROLL" },
{ 0, NULL },
};

static LRESULT CALLBACK tableSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	NMHDR *nmhdr = (NMHDR *) lParam;
	NMLVDISPINFOW *fill = (NMLVDISPINFO *) lParam;

	switch (uMsg) {
	case msgNOTIFY:
		switch (nmhdr->code) {
		case LVN_GETDISPINFO:
			tableGetCell((void *) data, &(fill->item));
			return 0;
		}
for(int i=0;lvnnames[i].code!=0;i++)if(lvnnames[i].code==nmhdr->code)printf("%s\n",lvnnames[i].name);
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	case WM_MOUSEMOVE:
		handle(hwnd, wParam, lParam, tableSetHot, (void *) data);
		// and let the list view do its thing
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:			// listviews have CS_DBLCICKS; check this to better mimic the behavior of a real checkbox
		handle(hwnd, wParam, lParam, tablePushed, (void *) data);
		// and let the list view do its thing
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	case WM_LBUTTONUP:
		handle(hwnd, wParam, lParam, tableToggled, (void *) data);
		// and let the list view do its thing
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	case WM_MOUSELEAVE:
		// TODO doesn't work
		tablePushed((void *) data, -1, -1);			// in case button held as drag out
		// and let the list view do its thing
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	// see table.autoresize() in table_windows.go for the column autosize policy
	case WM_NOTIFY:		// from the contained header control
		if (nmhdr->code == HDN_BEGINTRACK)
			tableStopColumnAutosize((void *) data);
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	case WM_NCDESTROY:
		if ((*fv_RemoveWindowSubclass)(hwnd, tableSubProc, id) == FALSE)
			xpanic("error removing Table subclass (which was for its own event handler)", GetLastError());
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	default:
		return (*fv_DefSubclassProc)(hwnd, uMsg, wParam, lParam);
	}
	xmissedmsg("Button", "tableSubProc()", uMsg);
	return 0;		// unreached
}

void setTableSubclass(HWND hwnd, void *data)
{
	if ((*fv_SetWindowSubclass)(hwnd, tableSubProc, 0, (DWORD_PTR) data) == FALSE)
		xpanic("error subclassing Table to give it its own event handler", GetLastError());
}

void tableAppendColumn(HWND hwnd, int index, LPWSTR name)
{
	LVCOLUMNW col;

	ZeroMemory(&col, sizeof (LVCOLUMNW));
	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
	col.fmt = LVCFMT_LEFT;
	col.pszText = name;
	col.iSubItem = index;
	col.iOrder = index;
	if (SendMessageW(hwnd, LVM_INSERTCOLUMN, (WPARAM) index, (LPARAM) (&col)) == (LRESULT) -1)
		xpanic("error adding column to Table", GetLastError());
}

void tableUpdate(HWND hwnd, int nItems)
{
	if (SendMessageW(hwnd, LVM_SETITEMCOUNT, (WPARAM) nItems, 0) == 0)
		xpanic("error setting number of items in Table", GetLastError());
}

void tableAddExtendedStyles(HWND hwnd, LPARAM styles)
{
	// the bits of WPARAM specify which bits of LPARAM to look for; having WPARAM == LPARAM ensures that only the bits we want to add are affected
	SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM) styles, styles);
}

void tableAutosizeColumns(HWND hwnd, int nColumns)
{
	int i;

	for (i = 0; i < nColumns; i++)
		if (SendMessageW(hwnd, LVM_SETCOLUMNWIDTH, (WPARAM) i, (LPARAM) LVSCW_AUTOSIZE_USEHEADER) == FALSE)
			xpanic("error resizing columns of results list view", GetLastError());
}

void tableSetCheckboxImageList(HWND hwnd)
{
	if (SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM) checkboxImageList) == (LRESULT) NULL)
;//TODO		xpanic("error setting image list", GetLastError());
	// TODO free old one here if any/different
	// thanks to Jonathan Potter (http://stackoverflow.com/questions/25354448/why-do-my-owner-data-list-view-state-images-come-up-as-blank-on-windows-xp)
	if (SendMessageW(hwnd, LVM_SETCALLBACKMASK, LVIS_STATEIMAGEMASK, 0) == FALSE)
		xpanic("error marking state image list as application-managed", GetLastError());
}

// because Go won't let me do C.WPARAM(-1)
intptr_t tableSelectedItem(HWND hwnd)
{
	return (intptr_t) SendMessageW(hwnd, LVM_GETNEXTITEM, (WPARAM) -1, LVNI_SELECTED);
}

void tableSelectItem(HWND hwnd, intptr_t index)
{
	LVITEMW item;
	LRESULT current;

	// via http://support.microsoft.com/kb/131284
	// we don't need to clear the other bits; Tables don't support cutting or drag/drop
	current = SendMessageW(hwnd, LVM_GETNEXTITEM, (WPARAM) -1, LVNI_SELECTED);
	if (current != (LRESULT) -1) {
		ZeroMemory(&item, sizeof (LVITEMW));
		item.mask = LVIF_STATE;
		item.state = 0;
		item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
		if (SendMessageW(hwnd, LVM_SETITEMSTATE, (WPARAM) current, (LPARAM) (&item)) == FALSE)
			xpanic("error deselecting current Table item", GetLastError());
	}
	if (index == -1)			// select nothing
		return;
	ZeroMemory(&item, sizeof (LVITEMW));
	item.mask = LVIF_STATE;
	item.state = LVIS_FOCUSED | LVIS_SELECTED;
	item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	if (SendMessageW(hwnd, LVM_SETITEMSTATE, (WPARAM) index, (LPARAM) (&item)) == FALSE)
		xpanic("error selecting new Table item", GetLastError());
}
