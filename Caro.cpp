
// Caro.cpp : Defines the entry point for the application.
#include "stdafx.h"
#include "Caro.h"
#include <vector>

// Cấu trúc lưu tọa độ một điểm trên bàn cờ
struct point
{
    short x;
    short y;
    int _danhgia; // 1 cho người, 2 cho máy
};

std::vector<point> history; // Stack lưu lịch sử nước đi

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                           // instance hiện thời
TCHAR szTitle[MAX_LOADSTRING];             // tiêu đề cửa sổ
TCHAR szWindowClass[MAX_LOADSTRING];       // tên lớp cửa sổ

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Các biến điều khiển game
bool pause = false;                       // Dừng trò chơi
bool user = true;                         // Người chơi đánh trước
bool choi_voi_may = true;                   // true: chơi với máy, false: chơi với người
bool isPlayer1Turn = true;                  // true: người 1 (X), false: người 2 (O)

// Các biến và mảng dùng trong trò chơi
enum Luotdanh { NGUOI, MAY };
Luotdanh _luot;                           // lượt đánh hiện tại
int BanCo[30][30];                        // Mảng lưu trạng thái bàn cờ
int max_dosau = 3;                        // Độ sâu kiểm tra cho thuật toán
bool Com_win = false;
bool Com_lose = false;
short ix;                                 // Tọa độ x nước đi của máy vừa đánh
short iy;                                 // Tọa độ y nước đi của máy vừa đánh
// Thêm khai báo global cho timer
const int TIME_LIMIT = 40;
// 40 giây timeout

int timerCount = 0;         // Biến đếm số giây đã trôi qua

// Mảng điểm tấn công và phòng ngự
short DiemTanCong[5] = { 0, 2, 18, 162, 1400 };
short DiemPhongNgu[5] = { 0, 1, 9, 81, 729 };

// Các biến dùng cho việc đánh giá nước đi (giả sử Caro_class được định nghĩa trong Caro.h)
point* _NuocWin[11];
short count;
point* _NuocKT[3];

// Định nghĩa hàm khởi tạo, tạo cửa sổ, v.v.
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    hInst = hInstance;
    hWnd = CreateWindow(szWindowClass, szTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        0, 0, 915, 900, NULL, NULL, hInstance, NULL);

    // Đặt giá trị mặc định cho bàn cờ (ví dụ đặt nước đầu tiên tại vị trí giữa)
    BanCo[12][12] = 2;
    ix = 12;
    iy = 12;

    if (!hWnd)
    {
        return FALSE;
    }

    // Đảm bảo cửa sổ chính có focus để nhận thông báo bàn phím
    SetFocus(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Hộp thoại chọn chế độ chơi
    int ret = MessageBox(NULL, TEXT("Bạn muốn chơi với ai?\nYES: Chơi với máy\nNO: Chơi với người\nCancel: Thoát"),
        TEXT("Chọn chế độ chơi"),
        MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);

    if (ret == IDYES)
        choi_voi_may = true;
    else if (ret == IDNO)
        choi_voi_may = false;
    else
        return FALSE; // Nếu Cancel thì thoát

    return TRUE;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CARO));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CARO);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

// Hàm vẽ chữ X
void DrawX(HDC hdc, int iY, int iX)
{
    HPEN hpen, OldPen;
    COLORREF color = RGB(223, 0, 41);
    hpen = CreatePen(PS_SOLID, 4, color);
    OldPen = (HPEN)SelectObject(hdc, hpen);
    int X1 = iX * 30 + 4;
    int X2 = (iX + 1) * 30 - 4;
    int Y1 = iY * 30 + 4;
    int Y2 = (iY + 1) * 30 - 4;
    MoveToEx(hdc, X1, Y1, NULL);
    LineTo(hdc, X2, Y2);
    MoveToEx(hdc, X2, Y1, NULL);
    LineTo(hdc, X1, Y2);
    SelectObject(hdc, OldPen);
    DeleteObject(hpen);
}

// Hàm vẽ hình tròn (O)
void DrawO(HDC hdc, int iY, int iX)
{
    HPEN hpen, OldPen;
    COLORREF color = RGB(0, 178, 191);
    hpen = CreatePen(PS_SOLID, 4, color);
    OldPen = (HPEN)SelectObject(hdc, hpen);
    int x1 = iX * 30 + 2;
    int x2 = (iX + 1) * 30 - 2;
    int y1 = iY * 30 + 2;
    int y2 = (iY + 1) * 30 - 2;
    Ellipse(hdc, x1, y1, x2, y2);
    SelectObject(hdc, OldPen);
    DeleteObject(hpen);
}

// Hàm vẽ bàn cờ
void DrawBoard(HDC hdc)
{
    HPEN hpen, OldPen;
    COLORREF color = RGB(0, 174, 114);
    hpen = CreatePen(PS_SOLID, 1, color);
    OldPen = (HPEN)SelectObject(hdc, hpen);
    // Vẽ các đường dọc
    for (int i = 0; i <= 900; i += 30)
    {
        MoveToEx(hdc, i, 0, NULL);
        LineTo(hdc, i, 900);
    }
    // Vẽ các đường ngang
    for (int i = 0; i <= 900; i += 30)
    {
        MoveToEx(hdc, 0, i, NULL);
        LineTo(hdc, 900, i);
    }
    SelectObject(hdc, OldPen);
    DeleteObject(hpen);
}

// Hàm reset lại game mới
void NewGame()
{
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 30; j++)
        {
            BanCo[i][j] = 0;
        }
    }
    _luot = NGUOI;
    BanCo[12][12] = 2;
    history.clear();
}

// Hàm kiểm tra thắng: xét 5 quân liên tiếp theo các hướng
int Find_Win(bool(*pointFunc)(int, int))
{
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 30; j++)
        {
            if (pointFunc(i, j) && pointFunc(i + 1, j) &&
                pointFunc(i + 2, j) && pointFunc(i + 3, j) &&
                pointFunc(i + 4, j))
                return 1;
            if (pointFunc(i, j) && pointFunc(i, j + 1) &&
                pointFunc(i, j + 2) && pointFunc(i, j + 3) &&
                pointFunc(i, j + 4))
                return 1;
            if (pointFunc(i, j) && pointFunc(i + 1, j + 1) &&
                pointFunc(i + 2, j + 2) && pointFunc(i + 3, j + 3) &&
                pointFunc(i + 4, j + 4))
                return 1;
            if (pointFunc(i, j) && pointFunc(i - 1, j + 1) &&
                pointFunc(i - 2, j + 2) && pointFunc(i - 3, j + 3) &&
                pointFunc(i - 4, j + 4))
                return 1;
        }
    }
    return 0;
}

// Hàm kiểm tra quân của máy
bool Com(int i, int j)
{
    if (i >= 0 && i < 30 && j >= 0 && j < 30)
        return (BanCo[i][j] == 2);
    return false;
}

// Hàm kiểm tra quân của người
bool User(int i, int j)
{
    if (i >= 0 && i < 30 && j >= 0 && j < 30)
        return (BanCo[i][j] == 1);
    return false;
}

// Các hàm dịch chuyển theo hướng
void hang_ngang(int& i, int& j) { j++; }
void hang_doc(int& i, int& j) { i++; }
void cheoquaphai(int& i, int& j) { i++; j++; }
void cheoquatrai(int& i, int& j) { i++; j--; }

// Giả sử Caro_class được định nghĩa đầy đủ trong Caro.h (với các hàm: Khoitao, set_value, get_value, max_value, min_value)
Caro_class caro;

// Hàm đánh giá nước cờ dựa trên các hướng và lượt chơi
void Tinh_gia_tri(void(*pointer)(int&, int&), Luotdanh _luot)
{
    int i, j;
    int gh_cot1 = 0, gh_cot2 = 0, gh_dong1 = 0, gh_dong2 = 0;
    if (pointer == hang_ngang)
        gh_cot2 = 4;
    if (pointer == hang_doc)
        gh_dong2 = 4;
    if (pointer == cheoquatrai) { gh_dong1 = 4; gh_cot2 = 4; }
    if (pointer == cheoquaphai) { gh_cot2 = 4; gh_dong2 = 4; }

    int CountCom, CountUser;
    for (int dong = gh_dong1; dong < 30 - gh_dong2; dong++)
    {
        for (int cot = gh_cot1; cot < 30 - gh_cot2; cot++)
        {
            CountCom = 0;
            CountUser = 0;
            for (i = 0, j = 0; (i < 5) && (j < 5); pointer(i, j))
            {
                if (BanCo[dong + i][cot + j] == 1)
                    CountUser++;
                if (BanCo[dong + i][cot + j] == 2)
                    CountCom++;
            }
            if (CountUser * CountCom == 0 && CountUser != CountCom)
            {
                for (i = 0, j = 0; (i < 5) && (j < 5); pointer(i, j))
                {
                    if (BanCo[dong + i][cot + j] == 0)
                    {
                        if (CountUser == 0)
                        {
                            if (_luot == NGUOI)
                                caro.set_value(dong + i, cot + j, -caro.get_value(dong + i, cot + j) + DiemPhongNgu[CountCom]);
                            else
                                caro.set_value(dong + i, cot + j, caro.get_value(dong + i, cot + j) + DiemTanCong[CountCom]);
                        }
                        if (CountCom == 0)
                        {
                            if (_luot == MAY)
                                caro.set_value(dong + i, cot + j, caro.get_value(dong + i, cot + j) + DiemPhongNgu[CountUser]);
                            else
                                caro.set_value(dong + i, cot + j, -caro.get_value(dong + i, cot + j) + DiemTanCong[CountUser]);
                            if (CountUser == 4 || CountCom == 4)
                                caro.set_value(dong + i, cot + j, caro.get_value(dong + i, cot + j) * 2);
                        }
                        if (_luot == NGUOI)
                        {
                            caro.set_value(dong + i, cot + j, -(caro.get_value(dong + i, cot + j)));
                        }
                    }
                }
            }
        }
    }
}

void danhgia(Luotdanh _luot)
{
    caro.Khoitao();
    Tinh_gia_tri(hang_ngang, _luot);
    Tinh_gia_tri(hang_doc, _luot);
    Tinh_gia_tri(cheoquatrai, _luot);
    Tinh_gia_tri(cheoquaphai, _luot);
}

int temp_user1;
int temp_com1;

// Thuật toán tìm nước đi của máy (sử dụng minimax)
void _min(int _dosau_hientai);
void _max(int _dosau_hientai)
{
    int max = -50000;
    danhgia(MAY);
    point* _Com = new point[3];
    for (int i = 0; i < 3; i++)
    {
        caro.max_value(_Com[i].x, _Com[i].y);
        caro.set_value(_Com[i].x, _Com[i].y, 0);
    }
    for (int i = 0; i < 3; i++)
    {
        point temp_Com = _Com[i];
        BanCo[temp_Com.x][temp_Com.y] = 2;
        if (Find_Win(Com))
        {
            max = 50000;
            BanCo[temp_Com.x][temp_Com.y] = 0;
            if (_dosau_hientai == 0)
            {
                iy = _Com[i].x;
                ix = _Com[i].y;
            }
            break;
        }
        _min(_dosau_hientai);
        BanCo[temp_Com.x][temp_Com.y] = 0;
        if (max < temp_com1)
        {
            if (_dosau_hientai == 0)
            {
                iy = _Com[i].x;
                ix = _Com[i].y;
            }
            max = temp_com1;
        }
    }
    temp_user1 = max;
}

void _min(int _dosau_hientai)
{
    int min = 50000;
    danhgia(NGUOI);
    point* _User = new point[3];
    for (int i = 0; i < 3; i++)
    {
        caro.min_value(_User[i].x, _User[i].y);
        _User[i]._danhgia = caro.get_value(_User[i].x, _User[i].y);
        caro.set_value(_User[i].x, _User[i].y, 0);
    }
    for (int i = 0; i < 3; i++)
    {
        point temp_user = _User[i];
        BanCo[temp_user.x][temp_user.y] = 1;
        if (Find_Win(User))
        {
            min = -50000;
            BanCo[temp_user.x][temp_user.y] = 0;
            break;
        }
        else if (_dosau_hientai < max_dosau)
        {
            _max(_dosau_hientai + 1);
        }
        BanCo[temp_user.x][temp_user.y] = 0;
        if (min > _User[i]._danhgia)
        {
            min = _User[i]._danhgia;
        }
    }
    temp_com1 = min;
}

// Hàm xử lý click chuột trái (vị trí nhận sự kiện click dựa trên lParam)
int Click_chuot_trai(LPARAM lParam)
{
    ix = LOWORD(lParam) / 30;
    iy = HIWORD(lParam) / 30;
    if (BanCo[iy][ix] == 0)
    {
        if (choi_voi_may)
        {
            _luot = MAY;
            BanCo[iy][ix] = 1; // Người đánh quân 1
        }
        else
        {
            BanCo[iy][ix] = isPlayer1Turn ? 1 : 2; // 1 = X, 2 = O
            isPlayer1Turn = !isPlayer1Turn;
        }
        // Ghi lại nước đi của người vào history
        point p;
        p.x = (short)iy;
        p.y = (short)ix;
        p._danhgia = BanCo[iy][ix];
        history.push_back(p);
        return 1;
    }
    return 0;
}

bool huongdi = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc, mHdc;
    RECT rct, rct1;
    HBRUSH brush;
    GetClientRect(hWnd, &rct);
    HBITMAP bitmap1, oldBitmap;

    switch (message)
    {case WM_CREATE:
    SetTimer(hWnd, 1, 1000, NULL); // Timer với ID = 1, cứ mỗi 1000ms
    break;

    //case WM_LBUTTONDOWN:
    //    if (_luot == NGUOI)
    //    {
    //        if (Click_chuot_trai(lParam) == 1 && !pause)
    //            InvalidateRect(hWnd, NULL, FALSE);
    //    }
    //    else
    //    {
    //        if (choi_voi_may)
    //        {
    //            if (Find_Win(User) == 0 && Find_Win(Com) == 0)
    //            {
    //                _max(0);
    //                BanCo[iy][ix] = 2;
    //                // Ghi lại nước đi của máy vào history
    //                {
    //                    point p;
    //                    p.x = (short)iy;
    //                    p.y = (short)ix;
    //                    p._danhgia = 2;
    //                    history.push_back(p);
    //                }
    //                _luot = NGUOI;
    //            }
    //            InvalidateRect(hWnd, NULL, FALSE);
    //        }
    //    }
    //    break;
    //    // Khởi tạo timer với ID là 1, và khoảng thời gian 1000 ms (1 giây)
    //    SetTimer(hWnd, 1, 1000, NULL);
    //    break;
    case WM_LBUTTONDOWN:
        if (_luot == NGUOI)
        {
            if (Click_chuot_trai(lParam) == 1 && !pause)
            {
                timerCount = 0;  // Reset timer sau nước đi của người chơi
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else
        {
            if (choi_voi_may)
            {
                if (Find_Win(User) == 0 && Find_Win(Com) == 0)
                {
                    _max(0);
                    BanCo[iy][ix] = 2;
                    // Ghi lại nước đi của máy vào history
                    {
                        point p;
                        p.x = (short)iy;
                        p.y = (short)ix;
                        p._danhgia = 2;
                        history.push_back(p);
                    }
                    _luot = NGUOI;
                    timerCount = 0;  // Reset timer sau nước đi của máy
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;

    case WM_TIMER:
    {
        timerCount++;
        if (timerCount >= TIME_LIMIT)
        {
            // Tạm dừng timer để tránh xử lý lặp lại
            KillTimer(hWnd, 1);

            if (choi_voi_may)  // Chế độ người chơi với máy (PvC)
            {
                if (_luot == NGUOI)
                {
                    MessageBox(hWnd, TEXT("Bạn đã thua do quá thời gian chờ (40 giây)!"),
                        TEXT("Thua cuộc"), MB_OK | MB_ICONINFORMATION);
                }
                else  // lượt máy
                {
                    MessageBox(hWnd, TEXT("Máy đã thua do quá thời gian chờ (40 giây)!"),
                        TEXT("Thua cuộc"), MB_OK | MB_ICONINFORMATION);
                }
            }
            else // Chế độ 2 người chơi (PvP)
            {
                if (isPlayer1Turn)
                {
                    MessageBox(hWnd, TEXT("Người chơi 1 (X) đã thua do quá thời gian chờ (40 giây)!"),
                        TEXT("Thua cuộc"), MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    MessageBox(hWnd, TEXT("Người chơi 2 (O) đã thua do quá thời gian chờ (40 giây)!"),
                        TEXT("Thua cuộc"), MB_OK | MB_ICONINFORMATION);
                }
            }
            // Reset game sau khi timeout
            NewGame();
            timerCount = 0;
            InvalidateRect(hWnd, NULL, FALSE);
            // Khởi động lại Timer cho ván mới
            SetTimer(hWnd, 1, 1000, NULL);
        }
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_NEWGAME:
            NewGame();
            pause = false;
            Com_win = false;
            ix = 12;
            iy = 12;
            // Hiển thị hộp thoại chọn chế độ chơi lại
            {
                int ret = MessageBox(NULL,
                    TEXT("Bạn muốn chơi với ai?\nYES: Chơi với máy\nNO: Chơi với người\nCancel: Thoát"),
                    TEXT("Chọn chế độ chơi"),
                    MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1);
                if (ret == IDYES) {
                    choi_voi_may = true;
                }
                else if (ret == IDNO) {
                    choi_voi_may = false;
                }
                else {
                    // Nếu người dùng nhấn Cancel thì có thể quay lại game cũ hoặc dừng game.
                    // Ở đây ta đặt pause = true để tạm dừng game
                    pause = true;
                }
            }
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case ID_UNDO:
            MessageBox(hWnd,
                TEXT("Hướng dẫn sử dụng Undo:\n\n")
                TEXT("1. Để quay lại nước đánh vừa thực hiện, nhấn phím Space.\n")
                TEXT("2. Trong chế độ chơi với máy (PvC), Undo sẽ xóa cả nước đi của người và của máy\n")
                TEXT("   (nếu có đủ dữ liệu), cho phép bạn thử lại nước đánh khác.\n")
                TEXT("3. Trong chế độ 2 người chơi (PvP), Undo sẽ xóa nước đi cuối cùng và đảo ngược lượt đánh.\n")
                TEXT("4. Lưu ý: Chức năng Undo chỉ hoạt động khi các nước đi được lưu đầy đủ\n")
                TEXT("   trong lịch sử (history).\n"),
                TEXT("Hướng dẫn Undo"),
                MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_ABOUT:
            MessageBox(hWnd,
                TEXT("Luật chơi Cờ Ca Rô:\n\n")
                TEXT("1. Hai người chơi lần lượt đánh X và O vào các ô trống.\n")
                TEXT("2. Người nào xếp được 5 quân liên tiếp theo hàng, cột hoặc chéo sẽ thắng.\n")
                TEXT("3. Trò chơi hòa nếu không còn nước đi.\n"),
                TEXT("Luật chơi"),
                MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_EXIT:
            if (MessageBox(hWnd, TEXT("Bạn có muốn thoát không?"), TEXT("Exit"), MB_OKCANCEL) == IDOK)
                DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_KEYDOWN:
        // Xử lý phím Space để Undo
        if (wParam == VK_SPACE)
        {
            // Debug: kiểm tra sự kiện phím đã nhận
            // MessageBox(hWnd, TEXT("Space pressed"), TEXT("Debug"), MB_OK);
            if (choi_voi_may)
            {
                // Undo 2 bước: xóa nước đi của máy và của người, nếu có đủ dữ liệu
                if (history.size() >= 2)
                {
                    point lastMove = history.back();
                    history.pop_back();
                    BanCo[lastMove.x][lastMove.y] = 0;
                    lastMove = history.back();
                    history.pop_back();
                    BanCo[lastMove.x][lastMove.y] = 0;
                    _luot = NGUOI;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                {
                    MessageBox(hWnd, TEXT("Không thể Undo!"), TEXT("Thông báo"), MB_OK);
                }
            }
            else  // Chế độ PvP
            {
                if (!history.empty())
                {
                    point lastMove = history.back();
                    history.pop_back();
                    BanCo[lastMove.x][lastMove.y] = 0;
                    isPlayer1Turn = !isPlayer1Turn;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                {
                    MessageBox(hWnd, TEXT("Không thể Undo!"), TEXT("Thông báo"), MB_OK);
                }
            }
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        mHdc = CreateCompatibleDC(hdc);
        bitmap1 = CreateCompatibleBitmap(hdc, rct.right - rct.left, rct.bottom - rct.top);
        oldBitmap = (HBITMAP)SelectObject(mHdc, (HBITMAP)bitmap1);
        FillRect(mHdc, &rct, (HBRUSH)(COLOR_WINDOW + 1));
        DrawBoard(mHdc);

        if (ix != -1 && iy != -1)
        {
            rct1.top = iy * 30 + 2;
            rct1.bottom = rct1.top + 28;
            rct1.left = ix * 30 + 2;
            rct1.right = rct1.left + 28;
            brush = CreateSolidBrush(RGB(254, 248, 134));
            FillRect(mHdc, &rct1, brush);
            DeleteObject(brush);
        }

        for (int i = 0; i < 30; i++)
        {
            for (int j = 0; j < 30; j++)
            {
                if (BanCo[i][j] == 1)
                    DrawX(mHdc, i, j);
                else if (BanCo[i][j] == 2)
                    DrawO(mHdc, i, j);
            }
        }

        BitBlt(hdc, 0, 0, 1000, 1000, mHdc, 0, 0, SRCCOPY);

        // Xử lý thông báo thắng thua cho chế độ PvP
        if (!choi_voi_may)
        {
            if (Find_Win(User) == 1)
            {
                if (MessageBox(hWnd, TEXT("Người chơi 1 (X) thắng!\nBạn có muốn chơi lại không?"),
                    TEXT("Chiến thắng"), MB_OKCANCEL) == IDOK)
                {
                    NewGame();
                    pause = false;
                    Com_win = false;
                    isPlayer1Turn = true;
                    ix = 12; iy = 12;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                    pause = true;
            }
            else if (Find_Win([](int i, int j) { return BanCo[i][j] == 2; }) == 1)
            {
                if (MessageBox(hWnd, TEXT("Người chơi 2 (O) thắng!\nBạn có muốn chơi lại không?"),
                    TEXT("Chiến thắng"), MB_OKCANCEL) == IDOK)
                {
                    NewGame();
                    pause = false;
                    Com_win = false;
                    isPlayer1Turn = true;
                    ix = 12; iy = 12;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                    pause = true;
            }
        }
        // Xử lý thắng thua cho chế độ PvC
        else
        {
            if (Find_Win(User) == 1)
            {
                if (MessageBox(hWnd, TEXT("Bạn đã thắng!\nBạn có muốn chơi lại không?"),
                    TEXT("Chiến thắng"), MB_OKCANCEL) == IDOK)
                {
                    NewGame();
                    pause = false;
                    Com_win = false;
                    ix = 12; iy = 12;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                    pause = true;
            }
            else if (Find_Win(Com) == 1)
            {
                if (MessageBox(hWnd, TEXT("Bạn đã thua!\nBạn có muốn chơi lại không?"),
                    TEXT("Thua cuộc"), MB_OKCANCEL) == IDOK)
                {
                    NewGame();
                    pause = false;
                    Com_win = false;
                    ix = 12; iy = 12;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                else
                    pause = true;
            }
            else
            {
                if (_luot == MAY)
                {
                    SendMessage(hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(84, 138));
                }
            }
        }

        SelectObject(mHdc, oldBitmap);
        DeleteObject(mHdc);
        DeleteObject(bitmap1);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Xử lý hộp thoại About
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_CARO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CARO));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
