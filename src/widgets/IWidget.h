#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>

namespace Widgets {

struct MouseEvent {
    enum class Type { Move, LeftDown, LeftUp, RightClick, Leave };
    Type type;
    int x;
    int y;
};

class IWidget {
public:
    virtual ~IWidget() = default;

    virtual std::wstring GetId() const = 0;
    virtual std::wstring GetName() const = 0;

    virtual void OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) = 0;
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnRender(ID2D1RenderTarget* pRT) = 0;
    virtual void OnMouseEvent(const MouseEvent& evt) = 0;

    virtual D2D1_RECT_F GetBounds() const = 0;
    virtual void SetPosition(float x, float y) = 0;
    virtual void SetSize(float width, float height) = 0;
    virtual D2D1_SIZE_F GetSize() const = 0;
    virtual D2D1_SIZE_F GetBaseSize() const = 0;
    virtual D2D1_SIZE_F GetMinSize() const = 0;

    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enable) = 0;
    virtual void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) = 0;
};

} // namespace Widgets
