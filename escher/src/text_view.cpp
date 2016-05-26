#include <escher/text_view.h>

TextView::TextView() : TextView(nullptr, 0.0f, 0.0f)
{
}

TextView::TextView(const char * text, float horizontalAlignment, float verticalAlignment) :
  ChildlessView(),
  m_text(text),
  m_horizontalAlignment(horizontalAlignment),
  m_verticalAlignment(verticalAlignment)
{
}

void TextView::setText(const char * text) {
  m_text = text;
}

void TextView::drawRect(KDRect rect) const {
  KDSize textSize = KDStringSize(m_text);
  KDPoint origin = {
    (KDCoordinate)(m_horizontalAlignment*(bounds().width - textSize.width)),
    (KDCoordinate)(m_verticalAlignment*(bounds().height - textSize.height))
  };
  KDDrawString(m_text, origin, 0);
}
