#include <omni/visual/Tuning.h>

#include <QOpenGLTexture>
#include <QOpenGLPixelTransferOptions>
#include <omni/util.h>
#include <omni/visual/util.h>
#include <omni/visual/Rectangle.h>

namespace omni
{
  namespace visual
  {
    std::unique_ptr<QOpenGLShaderProgram> Tuning::blendShader_;
    std::unique_ptr<QOpenGLShaderProgram> Tuning::testCardShader_;

    Tuning::Tuning(omni::proj::Tuning& _tuning) :
      tuning_(_tuning)
    {
      warpGrid_.reset(new visual::WarpGrid(tuning_.warpGrid()));
    }

    omni::proj::Tuning const& Tuning::tuning() const
    {
      return tuning_;
    }

    void Tuning::drawTestCard(int _id) const
    {
      if (!testCardShader_) return;

      visual::with_current_context([&](QOpenGLFunctions& _)
      {
        auto _color = tuning_.color();
        GLfloat _red = _color.redF();
        GLfloat _green = _color.greenF();
        GLfloat _blue = _color.blueF();

        testCardShader_->bind();
        testCardShader_->setUniformValue("resolution",GLfloat(tuning_.width()),GLfloat(tuning_.height()));
        testCardShader_->setUniformValue("test_color",_red,_green,_blue);
        testCardShader_->setUniformValue("projector_id",_id);

        Rectangle::draw();
        testCardShader_->release();
      });
    }

    void Tuning::drawWarpGrid() const
    {
      if (!warpGrid_) return;

      visual::with_current_context([this](QOpenGLFunctions& _)
      {
        _.glEnable(GL_TEXTURE_2D);
        _.glDisable(GL_BLEND);
        glColor4f(1.0,1.0,1.0,1.0);
        warpGrid_->draw();

        _.glDisable(GL_TEXTURE_2D);
        warpGrid_->drawLines();
        warpGrid_->drawHandles(tuning_.color(),tuningRect());
      });
    }

    void Tuning::update()
    {
      if (!QOpenGLContext::currentContext()) return;
        
      using omni::util::fileToStr;

      if (!testCardShader_) 
      {
        static QString _vertSrc = fileToStr(":/shaders/testcard.vert");
        static QString _fragmentSrc = fileToStr(":/shaders/testcard.frag");

        testCardShader_.reset(new QOpenGLShaderProgram());
        testCardShader_->addShaderFromSourceCode(QOpenGLShader::Vertex,_vertSrc);
        testCardShader_->addShaderFromSourceCode(QOpenGLShader::Fragment,_fragmentSrc);
        testCardShader_->link();
      }

      if (!blendShader_)
      {
        static QString _vertSrc = fileToStr(":/shaders/blend.vert");
        static QString _fragmentSrc = fileToStr(":/shaders/blend.frag");
        blendShader_.reset(new QOpenGLShaderProgram());
        blendShader_->addShaderFromSourceCode(QOpenGLShader::Vertex,_vertSrc);
        blendShader_->addShaderFromSourceCode(QOpenGLShader::Fragment,_fragmentSrc);
        blendShader_->link();
      }

      updateBlendTexture();
    }


    void Tuning::updateBlendTexture()
    {
      if (!QOpenGLContext::currentContext()) return;
 
      bool _reset = !blendTex_;

      if (!!blendTex_) 
      {
        _reset = blendTex_->width() != tuning_.width() || blendTex_->height() != tuning_.height();
      }

      if (_reset)
      {
        blendTex_.reset(new QOpenGLTexture(tuning_.blendMask().strokeBuffer().toQImage()));
      }
 
      auto& _blendMask = tuning().blendMask(); 
      glBindTexture(GL_TEXTURE_2D, blendTex_->textureId());
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, tuning_.width(), tuning_.height(), GL_ALPHA, GL_UNSIGNED_BYTE, _blendMask.strokeBufferData());
      glBindTexture(GL_TEXTURE_2D, 0);
    }


    void Tuning::drawCursor(QPointF const& _pos)
    {
      auto _rect = tuningRect();

      visual::with_current_context([&](QOpenGLFunctions& _)
      {
        _.glDisable(GL_LINE_SMOOTH);
        glColor4f(1.0,1.0,1.0,1.0);
        _.glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
        glBegin(GL_LINE_LOOP);

        float _r = tuning_.blendMask().brush().size() * 0.5 / tuning_.width();
        util::for_each_circle_point(24,_r,[&](size_t _i, const QPointF& _p)
        {
          glVertex2f(_p.x() + _pos.x(),_p.y() * (_rect.height()/_rect.width()) + _pos.y());
        });
        glEnd();
        _.glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        _.glEnable(GL_LINE_SMOOTH);
      });
    }

    void Tuning::drawBlendMask() const
    {
      if (!blendShader_) return;

      visual::with_current_context([&](QOpenGLFunctions& _)
      { 
        warpGrid_->draw();
   
        auto& _mask = tuning().blendMask();
        _.glEnable(GL_BLEND);
        _.glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 
        blendShader_->bind();
        {
          blendShader_->setUniformValue("top",_mask.topWidth());
          blendShader_->setUniformValue("right",_mask.rightWidth());
          blendShader_->setUniformValue("bottom",_mask.bottomWidth());
          blendShader_->setUniformValue("left",_mask.leftWidth());
          blendShader_->setUniformValue("gamma",_mask.gamma());
          blendShader_->setUniformValue("mask",GLfloat(-1.0));
          warpGrid_->draw();
        }
        blendShader_->release();

        _.glDisable(GL_BLEND);
        glColor4f(0.0,0.0,0.0,1.0);

        float _b = 4.0;
        // Draw masks for borders
        glBegin(GL_QUADS);
        {
          glVertex2f(-0.5,-0.5 - _b);
          glVertex2f(0.5-_b,-0.5);
          glVertex2f(0.5-_b,0.5);
          glVertex2f(-0.5,0.5 + _b);
          glVertex2f(-0.5 + _b,-0.5 - _b);
          glVertex2f(0.5,-0.5);
          glVertex2f(0.5,0.5);
          glVertex2f(-0.5 + _b,0.5 + _b);
          glVertex2f(-0.5 + _b,0.5 + _b);
          glVertex2f(0.5 - _b,0.5 + _b);
          glVertex2f(0.5 - _b,0.5);
          glVertex2f(-0.5 + _b,0.5);
          glVertex2f(-0.5 + _b,-0.5);
          glVertex2f(0.5 - _b,-0.5);
          glVertex2f(0.5 - _b,-0.5 - _b);
          glVertex2f(-0.5 + _b,-0.5 - _b);
        }
        glEnd();

        _.glEnable(GL_BLEND);
        _.glEnable(GL_TEXTURE_2D);
        blendTex_->bind();
        {
          glColor4f(0.0,0.0,0.0,1.0);
          _.glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
          glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
          Rectangle::drawFlipped();
        }
        blendTex_->release();
      });
    }

    QRectF Tuning::tuningRect() const
    {
      float _projAspect = float(tuning_.width()) / tuning_.height();
      float _left = -0.5,_right = 0.5,_bottom = -0.5,_top = 0.5;
      if (_projAspect > 1.0)
      {
        _top *= _projAspect;
        _bottom *=  _projAspect;
      }
      else
      {
        _left /= _projAspect;
        _right /= _projAspect;
      }
      return QRectF(QPointF(_left,_top),QPointF(_right,_bottom));
    }
  }
}