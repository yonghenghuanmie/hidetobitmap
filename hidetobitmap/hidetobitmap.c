#include <windows.h>
LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	HWND hwnd;WNDCLASS wndclass;MSG msg;
	wndclass.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
	wndclass.hCursor=LoadCursor(0,IDC_ARROW);
	wndclass.hIcon=LoadIcon(0,IDI_APPLICATION);
	wndclass.hInstance=hInstance;
	wndclass.lpfnWndProc=WndProc;
	wndclass.lpszClassName=TEXT("hidetobitmap");
	wndclass.lpszMenuName=TEXT("menu");
	wndclass.style=CS_HREDRAW|CS_VREDRAW;   
	wndclass.cbWndExtra=DLGWINDOWEXTRA;
	wndclass.cbClsExtra=0;
	RegisterClass(&wndclass);
	hwnd=CreateDialog(hInstance,TEXT("dialog"),0,WndProc);
	while(GetMessage(&msg,0,0,0))
	{
		if(!IsDialogMessage(hwnd,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.message;
}
int condition(HWND hwnd,BYTE *source)
{
	if(((BITMAPFILEHEADER*)source)->bfType!='MB')
	{
		MessageBox(hwnd,TEXT("该文件不是位图文件!"),TEXT("失败"),MB_ICONERROR|MB_OK);
		return 0;
	}
	(BITMAPFILEHEADER*)source=(BITMAPFILEHEADER*)source+1;
	if(((BITMAPINFOHEADER*)source)->biSize!=sizeof(BITMAPINFOHEADER))
	{
		MessageBox(hwnd,TEXT("只支持BITMAPINFOHEADER信息头的位图文件!"),TEXT("失败"),MB_ICONERROR|MB_OK);
		return 0;
	}
	if(((BITMAPINFOHEADER*)source)->biBitCount!=32)
	{
		MessageBox(hwnd,TEXT("只支持32位位图文件!"),TEXT("失败"),MB_ICONERROR|MB_OK);
		return 0;
	}
	return 1;
}
void pickupfile(HWND hwnd,OPENFILENAME filename)
{
	HANDLE hfile,hcreate;BYTE *source,*destination,*offset;UINT bytes,count,sum,i,j;TCHAR string[25],buffer[9];
	if(INVALID_HANDLE_VALUE==(hfile=CreateFile(filename.lpstrFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)))
		MessageBox(hwnd,TEXT("打开位图文件失败!"),TEXT("失败"),MB_ICONERROR|MB_OK);
	else
	{
		bytes=GetFileSize(hfile,0);
		source=calloc(bytes,1);
		ReadFile(hfile,source,bytes,&count,0);
		if(count!=bytes)
			MessageBox(hwnd,TEXT("打开位图文件失败!"),TEXT("失败"),MB_ICONERROR|MB_OK);
		else if(condition(hwnd,source))
		{
			offset=source+((BITMAPFILEHEADER*)source)->bfOffBits;
			offset+=3;
			bytes=offset[0]+(offset[4]<<8)+(offset[8]<<16)+(offset[12]<<24);
			if(bytes==0xFFFFFFFF)
				MessageBox(hwnd,TEXT("该文件未包含隐藏文件!"),TEXT("失败"),MB_ICONERROR|MB_OK);
			else
			{
				for(i=0,sum=bytes,count=48*sum+16,
					filename.Flags=OFN_OVERWRITEPROMPT,
					filename.lpstrFilter=TEXT("所有文件(*.*)\0*.*\0"),
					filename.lpstrDefExt=buffer;i<sum;i++)
				{
					bytes=offset[48*i+16]+(offset[48*i+16+4]<<8)+(offset[48*i+16+8]<<16)+(offset[48*i+16+12]<<24);
					destination=calloc(bytes,1);
					for(j=0;j<bytes;j++)
						*destination++=*(offset+count+4*j);
					destination-=bytes;
					for(j=0;j<9;j++)
						buffer[j]=0;
					for(j=0;j<8;j++)
						buffer[j]=*(offset+32+48*i+4*j);
					if(GetSaveFileName(&filename))
					{
						if(INVALID_HANDLE_VALUE==(hcreate=CreateFile(filename.lpstrFile,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0)))
						{
							wsprintf(string,TEXT("共计%d个隐藏文件\n提取第%d个隐藏文件失败!"),sum,i+1);
							MessageBox(hwnd,string,TEXT("失败"),MB_ICONHAND|MB_OK);
							break;
						}
						else
						{
							WriteFile(hcreate,destination,bytes,&j,0);
							if(j!=bytes)
							{
								wsprintf(string,TEXT("共计%d个隐藏文件\n提取第%d个隐藏文件失败!"),sum,i+1);
								MessageBox(hwnd,string,TEXT("失败"),MB_ICONHAND|MB_OK);
								break;
							}
							else
							{
								wsprintf(string,TEXT("共计%d个隐藏文件\n成功提取第%d个隐藏文件!"),sum,i+1);
								MessageBox(hwnd,string,TEXT("成功"),MB_ICONASTERISK|MB_OK);
							}
							CloseHandle(hcreate);
						}
					}
					free(destination);
					count+=4*bytes;
				}
			}
		}
		CloseHandle(hfile);
		free(source);
	}
}
char * getextension(TCHAR path[])
{
	int i,j;char *string;
	for(i=lstrlen(path)-1;i>=0;i--)
		if(path[i]=='.')
			break;
		string=calloc(1,8);
		for(j=0,i++;i<lstrlen(path);i++,j++)
			string[j]=(char)path[i];
		return string;
}
UINT* getfile(HWND hwnd,TCHAR path[8][MAX_PATH],BYTE *bitmap)
{
	UINT i,j=0,size[8],count;BYTE *file[9];TCHAR string[13];HANDLE hfile; static UINT send[2];char *buffer;
	for(i=0;i<8;i++)
	{
		size[i]=0;
		if(*path[i])
		{
			if(INVALID_HANDLE_VALUE==(hfile=CreateFile(path[i],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)))
			{
				wsprintf(string,TEXT("打开第%d个隐藏文件失败!"),i+1);
				MessageBox(hwnd,string,TEXT("失败"),MB_ICONERROR|MB_OK);
				return 0;
			}
			size[i]=GetFileSize(hfile,0);
			file[i]=calloc(size[i],1);
			ReadFile(hfile,file[i],size[i],&count,0);
			if(size[i]!=count)
			{
				wsprintf(string,TEXT("打开第%d个隐藏文件失败!"),i+1);
				MessageBox(hwnd,string,TEXT("失败"),MB_ICONERROR|MB_OK);
				return 0;
			}
			CloseHandle(hfile);
		}
	}
	count=0;
	for(i=0;i<8;i++)
		if(size[i])
		{
			count+=size[i]+12;
			j++;
		}
		count+=4;
		(BITMAPFILEHEADER*)bitmap=(BITMAPFILEHEADER*)bitmap+1;
		i=((BITMAPINFOHEADER*)bitmap)->biHeight*((BITMAPINFOHEADER*)bitmap)->biWidth;
		if(i<count)
		{
			MessageBox(hwnd,TEXT("该位图分辨率不足以存储该文件数据!"),TEXT("失败"),MB_ICONERROR|MB_OK);
			return 0;
		}
		file[8]=calloc(count,1);
		*((int*)file[8])++=j;
		for(i=0;i<8;i++)
			if(size[i])
			{
				*((int*)file[8])++=size[i];
				buffer=getextension(path[i]);
				CopyMemory(file[8],buffer,8);
				free(buffer);
				file[8]+=8;
			}
			for(i=0;i<8;i++)
				if(size[i])
				{
					CopyMemory(file[8],file[i],size[i]);
					file[8]+=size[i];
					free(file[i]);
				}
				file[8]-=count;
				(BYTE*)send[0]=file[8];
				send[1]=count;
				return send;
}
void hidetobitmap(HWND hwnd,TCHAR path[9][MAX_PATH])
{
	HANDLE hfile;UINT bytes,count,*temp;BYTE *file,*save,*offset;
	if(INVALID_HANDLE_VALUE==(hfile=CreateFile(path[0],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0)))
		MessageBox(hwnd,TEXT("打开位图文件失败!"),TEXT("失败"),MB_ICONERROR|MB_OK);
	else
	{
		bytes=GetFileSize(hfile,0);
		file=calloc(bytes,1);
		ReadFile(hfile,file,bytes,&count,0);
		if(count!=bytes)
			MessageBox(hwnd,TEXT("打开位图文件失败!"),TEXT("失败"),MB_ICONERROR|MB_OK);
		else if(condition(hwnd,file))
		{
			if(temp=getfile(hwnd,path[1],file))
			{
				save=(byte*)*temp;
				bytes=*(temp+1);
				offset=file+((BITMAPFILEHEADER*)file)->bfOffBits;
				offset+=3;
				for(count=0;count<bytes;count++)
				{
					*offset=*save;
					offset+=4;
					save++;
				}
				save-=bytes;
				free(save);
				CloseHandle(hfile);
				if(INVALID_HANDLE_VALUE==(hfile=CreateFile(path[0],GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0)))
					MessageBox(hwnd,TEXT("打开位图文件失败!"),TEXT("失败"),MB_ICONHAND|MB_OK);
				else
				{
					WriteFile(hfile,file,((BITMAPFILEHEADER*)file)->bfSize,&count,0);
					if(count!=((BITMAPFILEHEADER*)file)->bfSize)
						MessageBox(hwnd,TEXT("写入位图文件失败!"),TEXT("失败"),MB_ICONHAND|MB_OK);
					else
						MessageBox(hwnd,TEXT("隐藏文件成功!"),TEXT("成功"),MB_ICONASTERISK|MB_OK);
				}
			}
		}
		CloseHandle(hfile);
		free(file);
	}
}
void drawbutton(DRAWITEMSTRUCT *dis,char state)
{
	HPEN hpen;char i;COLORREF color[6];
	if(state==1)
	{
		color[0]=RGB(255,255,255);
		color[1]=RGB(105,105,105);
		color[2]=RGB(227,227,227);
		color[3]=RGB(160,160,160);
		color[4]=color[5]=RGB(240,240,240);
	}
	else if(state==2)
	{
		color[0]=color[1]=RGB(100,100,100);
		color[2]=color[3]=RGB(160,160,160);
		color[4]=color[5]=RGB(240,240,240);
	}
	else if(state==3)
	{
		color[0]=color[1]=RGB(100,100,100);
		color[2]=RGB(255,255,255);
		color[3]=RGB(105,105,105);
		color[4]=RGB(227,227,227);
		color[5]=RGB(160,160,160);
	}
	hpen=CreatePen(0,4,RGB(0,180,0));
	SelectObject(dis->hDC,hpen);
	MoveToEx(dis->hDC,dis->rcItem.left,(dis->rcItem.top+dis->rcItem.bottom)/2,0);
	LineTo(dis->hDC,dis->rcItem.right,(dis->rcItem.top+dis->rcItem.bottom)/2);
	MoveToEx(dis->hDC,(dis->rcItem.left+dis->rcItem.right)/2,dis->rcItem.top,0);
	LineTo(dis->hDC,(dis->rcItem.left+dis->rcItem.right)/2,dis->rcItem.bottom);
	for(i=0;i<3;i++)
	{
		hpen=CreatePen(0,1,color[i*2]);
		DeleteObject(SelectObject(dis->hDC,hpen));
		MoveToEx(dis->hDC,dis->rcItem.left+i,dis->rcItem.bottom-i-1,0);
		LineTo(dis->hDC,dis->rcItem.left+i,dis->rcItem.top+i);
		LineTo(dis->hDC,dis->rcItem.right-i-1,dis->rcItem.top+i);
		hpen=CreatePen(0,1,color[i*2+1]);
		DeleteObject(SelectObject(dis->hDC,hpen));
		LineTo(dis->hDC,dis->rcItem.right-i-1,dis->rcItem.bottom-i-1);
		LineTo(dis->hDC,dis->rcItem.left,dis->rcItem.bottom-i-1);
	}
	DeleteObject(hpen);
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	int i,j=0;static UINT id=8,cx,cy;DRAWITEMSTRUCT *dis;static OPENFILENAME filename;static TEXTMETRIC tm;HDC hdc;
	static TCHAR path[10][MAX_PATH],title[10][MAX_PATH],temp[2];static HINSTANCE hInstance;HWND hcontrol;LOGFONT logfont;
	static HFONT buttonfont,editfont;static SCROLLINFO si;
	switch (message)
	{
	case WM_CREATE:
		hInstance=((CREATESTRUCT*)lParam)->hInstance;
		cx=GetSystemMetrics(SM_CXSCREEN);
		cy=GetSystemMetrics(SM_CYSCREEN);
		MoveWindow(hwnd,cx/4,cy/4,600,350,0);
		cx=600;cy=350;
		memset(&filename,0,sizeof(OPENFILENAME));
		filename.lStructSize=sizeof(OPENFILENAME);
		filename.hwndOwner=hwnd;
		filename.nMaxFile=MAX_PATH;
		filename.nMaxFileTitle=MAX_PATH;
		filename.Flags=OFN_CREATEPROMPT;
		hdc=GetDC(hwnd);
		GetTextMetrics(hdc,&tm);
		ReleaseDC(hwnd,hdc);
		memset(&logfont,0,sizeof(LOGFONT));
		logfont.lfHeight=15;
		lstrcpy(logfont.lfFaceName,TEXT("MS Shell Dlg"));
		buttonfont=CreateFontIndirect(&logfont);
		logfont.lfHeight=20;
		editfont=CreateFontIndirect(&logfont);
		return 0;
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwnd,2),WM_SETFONT,(UINT)editfont,0);
		SendMessage(GetDlgItem(hwnd,4),WM_SETFONT,(UINT)editfont,0);
		return 0;
	case WM_DRAWITEM:
		dis=(DRAWITEMSTRUCT*)lParam;
		drawbutton(dis,1);
		if(dis->itemState&ODS_SELECTED)
			drawbutton(dis,2);
		else if(dis->itemState&ODS_FOCUS)
			drawbutton(dis,3);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case 1:
			filename.lpstrFile=path[0];
			filename.lpstrFileTitle=title[0];
			filename.lpstrFilter=TEXT("位图文件(*.bmp)\0*.bmp\0所有文件(*.*)\0*.*\0");
			if(GetOpenFileName(&filename))
				pickupfile(hwnd,filename);
			break;
		case 3:
			filename.lpstrFile=path[1];
			filename.lpstrFileTitle=title[1];
			filename.lpstrFilter=TEXT("位图文件(*.bmp)\0*.bmp\0所有文件(*.*)\0*.*\0");
			if(GetOpenFileName(&filename))
				SetWindowText(GetDlgItem(hwnd,2),path[1]);
			break;
		case 5:case 9:case 11:case 13:case 15:case 17:case 19:case 21:
			if(LOWORD(wParam)==5)
				i=2;
			else
				i=(LOWORD(wParam)-9)/2+3;
			filename.lpstrFile=path[i];
			filename.lpstrFileTitle=title[i];
			filename.lpstrFilter=TEXT("所有文件(*.*)\0*.*\0");
			if(GetOpenFileName(&filename))
				SetWindowText(GetDlgItem(hwnd,LOWORD(wParam)-1),path[i]);
			break;
		case 6:
			if(si.nPos)
				ScrollWindow(hwnd,0,si.nPos*31*tm.tmHeight/8,0,0);
			hcontrol=CreateWindowEx(WS_EX_CLIENTEDGE,TEXT("edit"),0,WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,
				14*tm.tmAveCharWidth/4,(id/2*31-59)*tm.tmHeight/8,229*tm.tmAveCharWidth/4,14*tm.tmHeight/8,
				hwnd,(HMENU)id,hInstance,0);
			SendMessage(hcontrol,WM_SETFONT,(UINT)editfont,0);
			hcontrol=CreateWindow(TEXT("button"),TEXT("选择"),WS_CHILD|WS_VISIBLE|WS_TABSTOP,248*tm.tmAveCharWidth/4,
				(id/2*31-59)*tm.tmHeight/8,50*tm.tmAveCharWidth/4,14*tm.tmHeight/8,hwnd,(HMENU)(id+1),hInstance,0);
			SendMessage(hcontrol,WM_SETFONT,(UINT)buttonfont,0);
			DestroyWindow(GetDlgItem(hwnd,6));
			CreateWindow(TEXT("button"),0,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,303*tm.tmAveCharWidth/4,
				(id/2*31-59)*tm.tmHeight/8,16*tm.tmAveCharWidth/4,14*tm.tmHeight/8,hwnd,(HMENU)(6),hInstance,0);
			DestroyWindow(GetDlgItem(hwnd,7));
			hcontrol=CreateWindow(TEXT("button"),TEXT("确定"),WS_CHILD|WS_VISIBLE|WS_TABSTOP,14*tm.tmAveCharWidth/4,
				(id/2*31-28)*tm.tmHeight/8,50*tm.tmAveCharWidth/4,14*tm.tmHeight/8,hwnd,(HMENU)(7),hInstance,0);
			SendMessage(hcontrol,WM_SETFONT,(UINT)buttonfont,0);
			if(si.nPos)
				ScrollWindow(hwnd,0,si.nPos*-31*tm.tmHeight/8,0,0);
			id+=2;
			if(id>=14)
			{
				DestroyWindow(GetDlgItem(hwnd,22));
				hcontrol=CreateWindow(TEXT("scrollbar"),0,WS_CHILD|WS_VISIBLE|WS_TABSTOP|SBS_VERT,cx-35,0,
					18,cy-58,hwnd,(HMENU)22,hInstance,0);
				si.cbSize=sizeof(SCROLLINFO);
				si.fMask=SIF_PAGE|SIF_RANGE|SIF_POS;
				si.nMin=0;
				si.nMax=(id-4)/2;
				si.nPage=5;
				SetScrollInfo(hcontrol,SB_CTL,&si,TRUE);
			}
			if(id==22)
				DestroyWindow(GetDlgItem(hwnd,6));
			break;
		case 7:
			if(!GetWindowText(GetDlgItem(hwnd,2),temp,2))
				MessageBox(hwnd,TEXT("请选择位图文件!"),TEXT("失败"),MB_ICONHAND|MB_OK);
			else
			{
				for(i=0;i<(int)id/2-3;i++)
					if(GetWindowText(GetDlgItem(hwnd,i?2*i+6:4),path[i+2],MAX_PATH))
						j=1;
					if(!j)
					{
						MessageBox(hwnd,TEXT("请选择隐藏文件!"),TEXT("失败"),MB_ICONHAND|MB_OK);
						break;
					}
					else
						hidetobitmap(hwnd,path[1]);
			}
			break;
		}
		return 0;
		case WM_VSCROLL:
			si.fMask=SIF_POS|SIF_TRACKPOS;
			hcontrol=GetDlgItem(hwnd,22);
			GetScrollInfo(hcontrol,SB_CTL,&si);
			i=si.nPos;
			switch(LOWORD(wParam))
			{
			case SB_LINEUP:
				si.nPos-=1;
				break;
			case SB_LINEDOWN:
				si.nPos+=1;
				break;
			case SB_PAGEUP:
				si.nPos-=5;
				break;
			case SB_PAGEDOWN:
				si.nPos+=5;
				break;
			case SB_TOP:
				si.nPos=0;
				break;
			case SB_BOTTOM:
				si.nPos=(id-4)/2;
				break;
			case SB_THUMBTRACK:
				si.nPos=si.nTrackPos;
				break;
			}
			si.fMask=SIF_POS;
			SetScrollInfo(hcontrol,SB_CTL,&si,TRUE);
			GetScrollInfo(hcontrol,SB_CTL,&si);
			if(i!=si.nPos)
			{
				i=si.nPos-i;
				ScrollWindow(hwnd,0,i*-31*tm.tmHeight/8,0,0);
				MoveWindow(hcontrol,cx-35,0,18,cy-58,TRUE);
			}
			return 0;
			case WM_MOUSEWHEEL:
				if(GetDlgItem(hwnd,22))
				{
					if((short)HIWORD(wParam)>0)
						SendMessage(hwnd,WM_VSCROLL,SB_LINEUP,0);
					else
						SendMessage(hwnd,WM_VSCROLL,SB_LINEDOWN,0);
				}
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
	}
return DefWindowProc(hwnd,message,wParam,lParam);
}