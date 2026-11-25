#include "MemoWidget.h"
#include "../core/InputBox.h"
#include <cwchar>
#include <algorithm>

namespace Widgets {

MemoWidget::MemoWidget(float x, float y, float width, float height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_baseWidth(220.0f)
    , m_baseHeight(135.0f)
    , m_minWidth(160.0f)
    , m_minHeight(85.0f)
    , m_enabled(true)
    , m_onChanged(nullptr)
    , m_hoveredIndex(-1)
    , m_hoverAddBtn(false)
    , m_hoverClearBtn(false)
    , m_hoverDeleteBtn(false)
    , m_mouseX(-1)
    , m_mouseY(-1)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTitle(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushTextDone(nullptr)
    , m_pBrushDivider(nullptr)
    , m_pBrushAccent(nullptr)
    , m_pBrushHover(nullptr)
    , m_pTitleFormat(nullptr)
    , m_pTextFormat(nullptr)
    , m_pSubFormat(nullptr)
    , m_pIconFormat(nullptr) {
    if (m_width <= 0) m_width = m_baseWidth;
    if (m_height <= 0) m_height = m_baseHeight;

    m_items = {
        { L"保持专注，优雅从容", false },
        { L"享受纯净美观的桌面空间", false },
        { L"Macify 开源桌面套件", true }
    };
    UpdateBaseHeight();
}

MemoWidget::~MemoWidget() {
    if (m_pIconFormat) m_pIconFormat->Release();
    if (m_pSubFormat) m_pSubFormat->Release();
    if (m_pTextFormat) m_pTextFormat->Release();
    if (m_pTitleFormat) m_pTitleFormat->Release();
    if (m_pBrushHover) m_pBrushHover->Release();
    if (m_pBrushAccent) m_pBrushAccent->Release();
    if (m_pBrushDivider) m_pBrushDivider->Release();
    if (m_pBrushTextDone) m_pBrushTextDone->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushTitle) m_pBrushTitle->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void MemoWidget::UpdateBaseHeight() {
    size_t count = m_items.empty() ? 1 : m_items.size();
    m_baseHeight = 44.0f + static_cast<float>(count) * 26.0f + 12.0f;
    if (m_baseHeight < 115.0f) m_baseHeight = 115.0f;
    m_minHeight = 85.0f;
}

void MemoWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    if (m_pBrushAccent) m_pBrushAccent->SetColor(accent);
}

void MemoWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.82f, 0.28f, 0.95f), &m_pBrushTitle); // Warm memo gold
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.96f, 0.98f, 0.95f), &m_pBrushText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.60f, 0.63f, 0.70f, 0.60f), &m_pBrushTextDone);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f), &m_pBrushDivider);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.25f, 0.65f, 1.0f, 0.95f), &m_pBrushAccent);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.08f), &m_pBrushHover);
    }

    if (pDWrite) {
        pDWrite->CreateTextFormat(
            L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"zh-CN", &m_pTitleFormat
        );

        pDWrite->CreateTextFormat(
            L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 11.0f, L"zh-CN", &m_pTextFormat
        );
        if (m_pTextFormat) {
            m_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWrite->CreateTextFormat(
            L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"zh-CN", &m_pSubFormat
        );
        if (m_pSubFormat) {
            m_pSubFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWrite->CreateTextFormat(
            L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 11.5f, L"zh-CN", &m_pIconFormat
        );
        if (m_pIconFormat) {
            m_pIconFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pIconFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void MemoWidget::OnUpdate(float deltaTime) {
    (void)deltaTime;
}

void MemoWidget::SetItems(const std::vector<MemoItem>& items) {
    m_items = items;
    UpdateBaseHeight();
    if (m_height < m_baseHeight) {
        m_height = m_baseHeight;
    }
}

void MemoWidget::AddItem(const std::wstring& text) {
    if (text.empty()) return;
    MemoItem item;
    item.text = text;
    item.done = false;
    m_items.push_back(item);
    UpdateBaseHeight();
    if (m_height < m_baseHeight) m_height = m_baseHeight;
    if (m_onChanged) m_onChanged(m_items);
}

void MemoWidget::ToggleItem(size_t index) {
    if (index < m_items.size()) {
        m_items[index].done = !m_items[index].done;
        if (m_onChanged) m_onChanged(m_items);
    }
}

void MemoWidget::RemoveItem(size_t index) {
    if (index < m_items.size()) {
        m_items.erase(m_items.begin() + index);
        UpdateBaseHeight();
        if (m_onChanged) m_onChanged(m_items);
    }
}

void MemoWidget::ClearDone() {
    m_items.erase(
        std::remove_if(m_items.begin(), m_items.end(), [](const MemoItem& it) { return it.done; }),
        m_items.end()
    );
    UpdateBaseHeight();
    if (m_onChanged) m_onChanged(m_items);
}

void MemoWidget::ResetDefaults() {
    m_items = {
        { L"保持专注，优雅从容", false },
        { L"享受纯净美观的桌面空间", false },
        { L"Macify 开源桌面套件", true }
    };
    UpdateBaseHeight();
    if (m_onChanged) m_onChanged(m_items);
}

void MemoWidget::CopyAllToClipboard() {
    if (m_items.empty()) return;

    std::wstring content;
    for (const auto& it : m_items) {
        content += it.done ? L"[✓] " : L"[ ] ";
        content += it.text + L"\r\n";
    }

    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        size_t bytes = (content.length() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            void* pMem = GlobalLock(hMem);
            if (pMem) {
                memcpy(pMem, content.c_str(), bytes);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
        CloseClipboard();
    }
}

void MemoWidget::ShowContextMenu(int screenX, int screenY) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 101, L"➕ 添加新待办备忘...");
    AppendMenuW(hMenu, MF_STRING, 102, L"🧹 清除已完成待办");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 103, L"📋 复制全部待办到剪贴板");
    AppendMenuW(hMenu, MF_STRING, 104, L"↺ 恢复默认示例便签");

    HWND hActive = GetActiveWindow();
    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, screenX, screenY, 0, hActive, nullptr);
    DestroyMenu(hMenu);

    if (cmd == 101) {
        std::wstring newText;
        if (Core::ShowInputBox(hActive, L"新建待办备忘", L"请输入新的备忘内容：", newText) && !newText.empty()) {
            AddItem(newText);
        }
    } else if (cmd == 102) {
        ClearDone();
    } else if (cmd == 103) {
        CopyAllToClipboard();
    } else if (cmd == 104) {
        ResetDefaults();
    }
}

void MemoWidget::OnRender(ID2D1RenderTarget* pRT) {
    if (!m_enabled || !pRT) return;

    D2D1_MATRIX_3X2_F oldTrans;
    pRT->GetTransform(&oldTrans);

    float scaleX = m_width / m_baseWidth;
    float scaleY = m_height / m_baseHeight;
    D2D1_MATRIX_3X2_F scaleTrans = D2D1::Matrix3x2F::Scale(scaleX, scaleY, D2D1::Point2F(m_x, m_y));
    pRT->SetTransform(scaleTrans * oldTrans);

    // 1. Card backdrop
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(m_x, m_y, m_x + m_baseWidth, m_y + m_baseHeight),
        16.0f,
        16.0f
    );
    if (m_pBrushBg) pRT->FillRoundedRectangle(cardRect, m_pBrushBg);
    if (m_pBrushBorder) pRT->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Title: "📝 灵感便签" + Badge
    if (m_pTitleFormat && m_pBrushTitle) {
        D2D1_RECT_F titleRect = D2D1::RectF(m_x + 14.0f, m_y + 11.0f, m_x + 95.0f, m_y + 29.0f);
        pRT->DrawTextW(L"📝 灵感便签", 6, m_pTitleFormat, titleRect, m_pBrushTitle);
    }

    // Done counter (e.g. 1/3)
    if (m_pSubFormat && m_pBrushTextDone && !m_items.empty()) {
        int doneCount = 0;
        for (const auto& it : m_items) if (it.done) doneCount++;
        wchar_t badge[32];
        swprintf_s(badge, L"%d/%d", doneCount, static_cast<int>(m_items.size()));
        D2D1_RECT_F badgeRect = D2D1::RectF(m_x + 90.0f, m_y + 12.0f, m_x + 130.0f, m_y + 28.0f);
        pRT->DrawTextW(badge, static_cast<UINT32>(wcslen(badge)), m_pSubFormat, badgeRect, m_pBrushTextDone);
    }

    // 3. Header Action buttons
    // [ + ] Button
    float addBtnRight = m_x + m_baseWidth - 14.0f;
    float addBtnLeft = addBtnRight - 20.0f;
    float addBtnTop = m_y + 10.0f;
    float addBtnBottom = addBtnTop + 20.0f;
    D2D1_ROUNDED_RECT addPill = D2D1::RoundedRect(D2D1::RectF(addBtnLeft, addBtnTop, addBtnRight, addBtnBottom), 4.0f, 4.0f);
    if (m_hoverAddBtn && m_pBrushHover) {
        pRT->FillRoundedRectangle(addPill, m_pBrushHover);
    }
    if (m_pIconFormat && m_pBrushAccent) {
        D2D1_RECT_F addRect = D2D1::RectF(addBtnLeft, addBtnTop, addBtnRight, addBtnBottom);
        pRT->DrawTextW(L"+", 1, m_pIconFormat, addRect, m_pBrushAccent);
    }

    // [ 🧹 ] Clean Done Button (if any done items)
    bool hasDone = false;
    for (const auto& it : m_items) if (it.done) { hasDone = true; break; }
    if (hasDone) {
        float clrRight = addBtnLeft - 6.0f;
        float clrLeft = clrRight - 20.0f;
        float clrTop = m_y + 10.0f;
        float clrBottom = clrTop + 20.0f;
        D2D1_ROUNDED_RECT clrPill = D2D1::RoundedRect(D2D1::RectF(clrLeft, clrTop, clrRight, clrBottom), 4.0f, 4.0f);
        if (m_hoverClearBtn && m_pBrushHover) {
            pRT->FillRoundedRectangle(clrPill, m_pBrushHover);
        }
        if (m_pSubFormat && m_pBrushTextDone) {
            D2D1_RECT_F clrRect = D2D1::RectF(clrLeft, clrTop, clrRight, clrBottom);
            pRT->DrawTextW(L"🧹", 2, m_pSubFormat, clrRect, m_pBrushTextDone);
        }
    }

    // 4. Divider line
    if (m_pBrushDivider) {
        pRT->DrawLine(
            D2D1::Point2F(m_x + 14.0f, m_y + 34.0f),
            D2D1::Point2F(m_x + m_baseWidth - 14.0f, m_y + 34.0f),
            m_pBrushDivider,
            1.0f
        );
    }

    // 5. Memo Content lines
    if (m_items.empty()) {
        if (m_pSubFormat && m_pBrushTextDone) {
            D2D1_RECT_F emptyRect = D2D1::RectF(m_x + 14.0f, m_y + 44.0f, m_x + m_baseWidth - 14.0f, m_y + 80.0f);
            pRT->DrawTextW(L"暂无待办，点击右上「+」或右键新增", 18, m_pSubFormat, emptyRect, m_pBrushTextDone);
        }
    } else {
        float curY = m_y + 40.0f;
        for (size_t i = 0; i < m_items.size(); ++i) {
            const auto& item = m_items[i];

            // Row hover background
            if (static_cast<int>(i) == m_hoveredIndex && m_pBrushHover) {
                D2D1_ROUNDED_RECT rowRect = D2D1::RoundedRect(
                    D2D1::RectF(m_x + 10.0f, curY - 2.0f, m_x + m_baseWidth - 10.0f, curY + 22.0f),
                    4.0f, 4.0f
                );
                pRT->FillRoundedRectangle(rowRect, m_pBrushHover);
            }

            // Checkbox
            float cbX = m_x + 22.0f;
            float cbY = curY + 10.0f;
            float cbR = 5.5f;

            if (item.done) {
                if (m_pBrushAccent) {
                    pRT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cbX, cbY), cbR, cbR), m_pBrushAccent);
                }
                if (m_pIconFormat && m_pBrushBg) {
                    D2D1_RECT_F checkRect = D2D1::RectF(cbX - cbR, cbY - cbR, cbX + cbR, cbY + cbR);
                    pRT->DrawTextW(L"✓", 1, m_pIconFormat, checkRect, m_pBrushBg);
                }
            } else {
                if (m_pBrushBorder) {
                    pRT->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cbX, cbY), cbR, cbR), m_pBrushBorder, 1.2f);
                }
            }

            // Text
            float textLeft = m_x + 34.0f;
            float textRight = m_x + m_baseWidth - 26.0f;
            D2D1_RECT_F textRect = D2D1::RectF(textLeft, curY, textRight, curY + 20.0f);
            ID2D1SolidColorBrush* pTxtBrush = item.done ? m_pBrushTextDone : m_pBrushText;
            if (m_pTextFormat && pTxtBrush) {
                pRT->DrawTextW(
                    item.text.c_str(),
                    static_cast<UINT32>(item.text.length()),
                    m_pTextFormat,
                    textRect,
                    pTxtBrush
                );
            }

            // Strikethrough if done
            if (item.done && pTxtBrush) {
                float lineLen = (std::min)(static_cast<float>(item.text.length()) * 11.5f, textRight - textLeft);
                pRT->DrawLine(
                    D2D1::Point2F(textLeft, curY + 10.0f),
                    D2D1::Point2F(textLeft + lineLen, curY + 10.0f),
                    pTxtBrush,
                    1.2f
                );
            }

            // Delete '×' button on hover
            if (static_cast<int>(i) == m_hoveredIndex) {
                float delX = m_x + m_baseWidth - 20.0f;
                float delY = curY + 1.0f;
                D2D1_RECT_F delRect = D2D1::RectF(delX, delY, delX + 14.0f, delY + 18.0f);
                if (m_pIconFormat && m_pBrushTextDone) {
                    pRT->DrawTextW(L"×", 1, m_pIconFormat, delRect, m_hoverDeleteBtn ? m_pBrushTitle : m_pBrushTextDone);
                }
            }

            curY += 24.0f;
        }
    }

    pRT->SetTransform(oldTrans);
}

void MemoWidget::OnMouseEvent(const MouseEvent& evt) {
    float scaleX = (m_width > 0 && m_baseWidth > 0) ? (m_width / m_baseWidth) : 1.0f;
    float scaleY = (m_height > 0 && m_baseHeight > 0) ? (m_height / m_baseHeight) : 1.0f;

    float localX = m_x + (static_cast<float>(evt.x) - m_x) / scaleX;
    float localY = m_y + (static_cast<float>(evt.y) - m_y) / scaleY;

    if (evt.type == MouseEvent::Type::Move) {
        m_mouseX = evt.x;
        m_mouseY = evt.y;

        // Header buttons
        float addBtnRight = m_x + m_baseWidth - 14.0f;
        float addBtnLeft = addBtnRight - 20.0f;
        m_hoverAddBtn = (localX >= addBtnLeft && localX <= addBtnRight && localY >= m_y + 10.0f && localY <= m_y + 30.0f);

        float clrRight = addBtnLeft - 6.0f;
        float clrLeft = clrRight - 20.0f;
        m_hoverClearBtn = (localX >= clrLeft && localX <= clrRight && localY >= m_y + 10.0f && localY <= m_y + 30.0f);

        // Row items
        m_hoveredIndex = -1;
        m_hoverDeleteBtn = false;
        float curY = m_y + 40.0f;
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (localY >= curY - 2.0f && localY <= curY + 22.0f && localX >= m_x + 10.0f && localX <= m_x + m_baseWidth - 10.0f) {
                m_hoveredIndex = static_cast<int>(i);
                float delX = m_x + m_baseWidth - 22.0f;
                if (localX >= delX - 4.0f && localX <= m_x + m_baseWidth - 8.0f) {
                    m_hoverDeleteBtn = true;
                }
                break;
            }
            curY += 24.0f;
        }
    } else if (evt.type == MouseEvent::Type::LeftUp) {
        // Click Add Button
        float addBtnRight = m_x + m_baseWidth - 14.0f;
        float addBtnLeft = addBtnRight - 20.0f;
        if (localX >= addBtnLeft && localX <= addBtnRight && localY >= m_y + 10.0f && localY <= m_y + 30.0f) {
            std::wstring newText;
            if (Core::ShowInputBox(nullptr, L"新建待办备忘", L"请输入新的备忘内容：", newText) && !newText.empty()) {
                AddItem(newText);
            }
            return;
        }

        // Click Clear Done Button
        float clrRight = addBtnLeft - 6.0f;
        float clrLeft = clrRight - 20.0f;
        if (localX >= clrLeft && localX <= clrRight && localY >= m_y + 10.0f && localY <= m_y + 30.0f) {
            ClearDone();
            return;
        }

        // Click Row item
        float curY = m_y + 40.0f;
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (localY >= curY - 2.0f && localY <= curY + 22.0f && localX >= m_x + 10.0f && localX <= m_x + m_baseWidth - 10.0f) {
                float delX = m_x + m_baseWidth - 22.0f;
                if (localX >= delX - 4.0f && localX <= m_x + m_baseWidth - 8.0f) {
                    // Clicked delete button
                    RemoveItem(i);
                } else {
                    // Clicked checkbox or text -> toggle status
                    ToggleItem(i);
                }
                return;
            }
            curY += 24.0f;
        }
    } else if (evt.type == MouseEvent::Type::RightClick) {
        ShowContextMenu(evt.x, evt.y);
    }
}

} // namespace Widgets
