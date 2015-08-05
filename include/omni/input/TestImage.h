#ifndef OMNI_INPUT_TESTIMAGE_H_
#define OMNI_INPUT_TESTIMAGE_H_

#include <QOpenGLShaderProgram>
#include "Interface.h"

namespace omni
{
  namespace input
  {
    class TestImage : 
      public Interface,
      private Registrar<TestImage>
    {
    public:
      OMNI_TYPEID("TestImage")

      TestImage();
      ~TestImage();

      GLuint textureId() const;
      void free();
      void update();
      
      inline bool acceptMapping() const { return false; }

      QPointF rulerPos() const;
      void setRulerPos(QPointF const&);
     
      QSize size() const;

      void toStream(QDataStream&) const;
      void fromStream(QDataStream&);

    private:
      QPointF pos_;
      std::unique_ptr<QOpenGLShaderProgram> shader_;
    };

  }
}

#endif /* OMNI_INPUT_TESTIMAGE_H_ */
