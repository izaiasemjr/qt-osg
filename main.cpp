#include <QApplication>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDesktopWidget>
#include <QScreen>
#include <QtGlobal>
#include <QWindow>
#include <QVector3D>

#include <osg/ref_ptr>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>
#include <osg/Camera>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/Material>
#include <osgGA/EventQueue>
#include <osgGA/TrackballManipulator>
#include <osg/Node>
#include <iostream>
#include <stdio.h>
#include "lib3mf_implicit.hpp"
#include <QDebug>


class M_IO {
  public:   
      static void readMesh(std::string pathFile, std::vector<std::vector<osg::ref_ptr<osg::Vec3Array>>> &meshes){
        using namespace Lib3MF;

        // This is very important to avoid lib3mf problems with conversions double/float from string
        // inside contexts that change locale
        std::locale::global(std::locale::classic()); 

        // Extract Extension of filename
        std::string sReaderName;
        std::string sWriterName;
        std::string sNewExtension;

        auto idx = pathFile.rfind('.');    
        std::string sExtension = (idx!= std::string::npos)? pathFile.substr(idx):"";

        std::transform(sExtension.begin(), sExtension.end(), sExtension.begin(), ::tolower);

        // Which Reader and Writer classes do we need?
        if (sExtension == ".stl") {
          sReaderName = "stl";
        }
        if (sExtension == ".3mf") {
          sReaderName = "3mf";
        }
        if (sReaderName.length() == 0) {
          // qDebug() << "unknown input file extension:" << sExtension ;
          exit(1);
        }

        // Import Model from 3MF File
        PWrapper wrapper = CWrapper::loadLibrary();
        PModel modelIn = wrapper->CreateModel();
        PReader reader = modelIn->QueryReader(sReaderName);
        // And deactivate the strict mode (default is "false", anyway. This just demonstrates where/how to use it).
        reader->SetStrictModeActive(false);
        reader->ReadFromFile(pathFile.c_str());
        
        for (Lib3MF_uint32 iWarning = 0; iWarning < reader->GetWarningCount(); iWarning++) {
          Lib3MF_uint32 nErrorCode;
          std::string sWarningMessage = reader->GetWarning(iWarning, nErrorCode);
          // std::cout << "Encountered warning #" << nErrorCode << " : " << sWarningMessage << std::endl;
        }
        
        PObjectIterator objectIterator = modelIn->GetObjects();
        std::vector<osg::ref_ptr<osg::Vec3Array>> mesh;    
        while (objectIterator->MoveNext()) {
          PObject object = objectIterator->GetCurrentObject();
          if (object->IsMeshObject()) {  
            std::vector<Lib3MF::sPosition> vertices; 
            modelIn->GetMeshObjectByID(object->GetResourceID())->GetVertices(vertices);

            std::vector<Lib3MF::sTriangle> trianglesIndices;
            modelIn->GetMeshObjectByID(object->GetResourceID())->GetTriangleIndices(trianglesIndices);

            // for (std::size_t i = 0; i<vertices.size();i++){
            //   mesh.addPoint(QVector3D(vertices[i].m_Coordinates[0],vertices[i].m_Coordinates[1],vertices[i].m_Coordinates[2]),color); 
            // }
            osg::ref_ptr<osg::Vec3Array> m_colors = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> m_vertices = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> m_normals = new osg::Vec3Array;
            for (std::size_t i = 0; i<trianglesIndices.size();i++){
              auto idx_vertex1 = trianglesIndices[i].m_Indices[0];
              auto vertex1 = QVector3D( (vertices[idx_vertex1].m_Coordinates[0]),
                                        (vertices[idx_vertex1].m_Coordinates[1]),
                                          (vertices[idx_vertex1].m_Coordinates[2]));

              auto idx_vertex2 = trianglesIndices[i].m_Indices[1]; 
              auto vertex2 = QVector3D(vertices[idx_vertex2].m_Coordinates[0],
                                          vertices[idx_vertex2].m_Coordinates[1],
                                          vertices[idx_vertex2].m_Coordinates[2]);

              auto idx_vertex3 = trianglesIndices[i].m_Indices[2];     
              auto vertex3 = QVector3D(vertices[idx_vertex3].m_Coordinates[0],
                                          vertices[idx_vertex3].m_Coordinates[1],
                                          vertices[idx_vertex3].m_Coordinates[2]);

              m_vertices->push_back(osg::Vec3(vertex1.x(),vertex1.y(),vertex1.z()));
              m_vertices->push_back(osg::Vec3(vertex2.x(),vertex2.y(),vertex2.z()));
              m_vertices->push_back(osg::Vec3(vertex3.x(),vertex3.y(),vertex3.z()));

              QVector3D Vn =  QVector3D::normal(vertex1,vertex2,vertex3);  
              m_normals->push_back(osg::Vec3(Vn.x(),Vn.y(),Vn.z()));
              m_normals->push_back(osg::Vec3(Vn.x(),Vn.y(),Vn.z()));
              m_normals->push_back(osg::Vec3(Vn.x(),Vn.y(),Vn.z()));

              m_colors->push_back(osg::Vec3(0.5,0.5,0.5));
              m_colors->push_back(osg::Vec3(0.5,0.5,0.5));
              m_colors->push_back(osg::Vec3(0.5,0.5,0.5));


            }
            mesh.push_back(m_vertices);
            m_vertices.release();
            mesh.push_back(m_colors);
            m_colors.release();
            mesh.push_back(m_normals);
            m_normals.release();
          }

        meshes.push_back(mesh);
        mesh.clear();
        
        }
      }

};

class QtOSGWidget : public QOpenGLWidget
{
public:
  osg::Geode* sceneGeode;
  osg::Material::ColorMode testMaterial;
  osg::Vec4 testColor;
  osg::Vec4 red = osg::Vec4(0.8, 0.1, 0.1, 1.0);
  osg::Vec4 blue = osg::Vec4(0.1, 0.1, 0.8, 1.0);
  bool markMeh=false;

  QtOSGWidget(qreal scaleX, qreal scaleY, QWidget* parent = 0)
      : QOpenGLWidget(parent)
        , _mGraphicsWindow(new osgViewer::GraphicsWindowEmbedded( this->x(), this->y(),
                                                                 this->width(), this->height() ) )
        , _mViewer(new osgViewer::Viewer)
      , m_scaleX(scaleX)
      , m_scaleY(scaleY)
      {

        sceneGeode = createTestScene();
        osg::Camera* camera = new osg::Camera;
        camera->setViewport( 0, 0, this->width(), this->height() );
        camera->setClearColor( osg::Vec4( 0.9f, 0.9f, 1.f, 1.f ) );
        float aspectRatio = static_cast<float>( this->width()) / static_cast<float>( this->height() );
        camera->setProjectionMatrixAsPerspective( 30.f, aspectRatio, 1.f, 1000.f );
        camera->setGraphicsContext( _mGraphicsWindow );

        _mViewer->setCamera(camera);
        _mViewer->setSceneData(sceneGeode);

        
        osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator;
        manipulator->setAllowThrow( true );
        

        this->setMouseTracking(true);
        _mViewer->setCameraManipulator(manipulator);
        _mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
        _mViewer->realize();
        _mViewer->setLightingMode(osg::View::LightingMode::HEADLIGHT);
        
        startTimer(1000);

      }
  virtual ~QtOSGWidget(){}
  void setScale(qreal X, qreal Y)
  {
      m_scaleX = X;
      m_scaleY = Y;
      this->resizeGL(this->width(), this->height());
  }
 
protected:
  virtual void paintGL() {
    _mViewer->frame();
  }
  virtual void resizeGL( int width, int height ) {
      this->getEventQueue()->windowResize(this->x()*m_scaleX, this->y() * m_scaleY, width*m_scaleX, height*m_scaleY);
      _mGraphicsWindow->resized(this->x()*m_scaleX, this->y() * m_scaleY, width*m_scaleX, height*m_scaleY);
      osg::Camera* camera = _mViewer->getCamera();
      camera->setViewport(0, 0, this->width()*m_scaleX, this->height()* m_scaleY);
  }
  virtual void initializeGL(){
      osg::Geode* geode = dynamic_cast<osg::Geode*>(_mViewer->getSceneData());

      osg::StateSet* stateSet = geode->getOrCreateStateSet();
      osg::Material* material = new osg::Material;
      material->setColorMode( osg::Material::DIFFUSE );
      stateSet->setAttributeAndModes( material, osg::StateAttribute::ON );
      stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::ON );
      stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

  }
  virtual void mouseMoveEvent(QMouseEvent* event){
      this->getEventQueue()->mouseMotion(event->x()*m_scaleX, event->y()*m_scaleY);
  }
  virtual void mousePressEvent(QMouseEvent* event){
      unsigned int button = 0;
      switch (event->button()){
      case Qt::RightButton:{
          markMeh=!markMeh;
          osg::ref_ptr<osg::Material> mat2 = new osg::Material;
          mat2->setDiffuse (osg::Material::FRONT_AND_BACK, markMeh?osg::Vec4(0.1, 0.8, 0.1, 1.0):testColor);
          sceneGeode->getChild(0)->asDrawable()->getOrCreateStateSet()->setAttributeAndModes(mat2, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);    
          button = 3;
      }
          break;
      case Qt::MiddleButton:
          button = 2;
          break;
      case Qt::LeftButton:
          button = 1;
          break;
      default:
          break;
      }
      this->getEventQueue()->mouseButtonPress(event->x()*m_scaleX, event->y()*m_scaleY, button);
  }
  virtual void mouseReleaseEvent(QMouseEvent* event){
      unsigned int button = 0;
      switch (event->button()){
      case Qt::LeftButton:
          button = 1;
          break;
      case Qt::MiddleButton:
          button = 2;
          break;
      case Qt::RightButton:
          button = 3;
          break;
      default:
          break;
      }
      this->getEventQueue()->mouseButtonRelease(event->x()*m_scaleX, event->y()*m_scaleY, button);
  }
  virtual void wheelEvent(QWheelEvent* event){
      int delta = event->delta();
      osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?
                  osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
      this->getEventQueue()->mouseScroll(motion);
  }
  virtual bool event(QEvent* event){
      bool handled = QOpenGLWidget::event(event);
      this->update();
      return handled;
  }
  void timerEvent(QTimerEvent *event){
    
      qDebug()<<"changing color material...";
      testColor = testColor == red? blue:red;
      for(int i=0; i<sceneGeode->getNumChildren();i++){
        if(markMeh && i==0) continue;
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse (osg::Material::FRONT_AND_BACK, testColor);
        sceneGeode->getChild(i)->asDrawable()->getOrCreateStateSet()->setAttributeAndModes(mat, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);    
       
      }
      this->update();

  } 

private:
  osgGA::EventQueue* getEventQueue() const {
    osgGA::EventQueue* eventQueue = _mGraphicsWindow->getEventQueue();
    return eventQueue;
  }
  osg::Geode* createTestScene(){
    
    // openMesh STL
    std::vector<std::vector<osg::ref_ptr<osg::Vec3Array>>> meshes;
    M_IO::readMesh("../data/BabyYoda.stl",meshes);
   
    
    // open mesh 3MF
    M_IO::readMesh("../data/cube_gears.3mf",meshes);
   
    // Scene
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    for(int i=0;i<meshes.size();i++){
      // geometry
      osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;  
      geom->setVertexArray(meshes[i][0].get()); //vertex
      geom->setColorArray(meshes[i][1].get());  // colors
      geom->setNormalArray(meshes[i][2].get()); // normals

      //  properties
      geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
      geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
      
      // primitives for paint
      geom->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, meshes[i][0]->size()));
      
      // add  mesh in scene
      geode->addDrawable(geom.get());
    }

    // add  cylinder in scene
    osg::Cone* cylinder    = new osg::Cone( osg::Vec3( 120.f, 1.f, 1.f ), 30.25f, 70.5f );
    osg::ShapeDrawable* cyl = new osg::ShapeDrawable( cylinder );
    cyl->setColor( osg::Vec4( 0.8f, 0.5f, 0.2f, 1.f ) );       
    geode->addDrawable(cyl);

    // add  capsule in scene
    osg::Capsule* capsule    = new osg::Capsule( osg::Vec3( -100.f, -1.f, -1.f ), 25.25f,40.5f );
    osg::ShapeDrawable* cp = new osg::ShapeDrawable( capsule );
    geode->addDrawable(cp);

    return geode.release();    
}    
  
  osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _mGraphicsWindow;
  osg::ref_ptr<osgViewer::Viewer> _mViewer;
  qreal m_scaleX, m_scaleY;
};

int main(int argc, char** argv)
{
    QApplication qapp(argc, argv);

    QMainWindow window;
    QtOSGWidget* widget = new QtOSGWidget(1, 1, &window);
    window.setCentralWidget(widget);
    window.show();

    return qapp.exec();
}
