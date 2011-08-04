#include "Vizkit3DWidget.hpp"
#include <QVBoxLayout>
#include <GridNode.hpp>
#include <vizkit/MotionCommandVisualization.hpp>
#include <vizkit/TrajectoryVisualization.hpp>
#include <vizkit/WaypointVisualization.hpp>
#include <vizkit/RigidBodyStateVisualization.hpp>

using namespace vizkit;

Vizkit3DWidget::Vizkit3DWidget( QWidget* parent, Qt::WindowFlags f )
    : CompositeViewerQOSG( parent, f )
{
    createSceneGraph();

    QWidget* viewWidget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget( viewWidget );
    this->setLayout( layout );

    view = new ViewQOSG( viewWidget );
    view->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    view->setData( root );
    addView( view );

    // pickhandler is for selecting objects in the opengl view
    pickHandler = new PickHandler();
    view->addEventHandler( pickHandler );
    
    // add visualization of ground grid 
    GridNode *gn = new GridNode();
    root->addChild(gn);
    
    pluginNames = new QStringList;
}

QSize Vizkit3DWidget::sizeHint() const
{
    return QSize( 800, 600 );
}

osg::ref_ptr<osg::Group> Vizkit3DWidget::getRootNode() const
{
    return root;
}

void Vizkit3DWidget::setTrackedNode( VizPluginBase* plugin )
{
    view->setTrackedNode(plugin->getVizNode());
}

void Vizkit3DWidget::createSceneGraph() 
{
    //create root node that holds all other nodes
    root = new osg::Group;
    
    osg::ref_ptr<osg::StateSet> state = root->getOrCreateStateSet();
    state->setGlobalDefaults();
    state->setMode( GL_LINE_SMOOTH, osg::StateAttribute::ON );
    state->setMode( GL_POINT_SMOOTH, osg::StateAttribute::ON );
    state->setMode( GL_BLEND, osg::StateAttribute::ON );    
    state->setMode( GL_DEPTH_TEST, osg::StateAttribute::ON);
    state->setMode( GL_LIGHTING, osg::StateAttribute::ON );
    state->setMode( GL_LIGHT0, osg::StateAttribute::ON );
    state->setMode( GL_LIGHT1, osg::StateAttribute::ON );
	
    root->setDataVariance(osg::Object::DYNAMIC);

    // Add the Light to a LightSource. Add the LightSource and
    //   MatrixTransform to the scene graph.
    for(size_t i=0;i<2;i++)
    {
	osg::ref_ptr<osg::Light> light = new osg::Light;
	light->setLightNum(i);
	switch(i) {
	    case 0:
		light->setAmbient( osg::Vec4( .1f, .1f, .1f, 1.f ));
		light->setDiffuse( osg::Vec4( .8f, .8f, .8f, 1.f ));
		light->setSpecular( osg::Vec4( .8f, .8f, .8f, 1.f ));
		light->setPosition( osg::Vec4( 1.f, 1.5f, 2.f, 0.f ));
		break;
	    case 1:
		light->setAmbient( osg::Vec4( .1f, .1f, .1f, 1.f ));
		light->setDiffuse( osg::Vec4( .1f, .3f, .1f, 1.f ));
		light->setSpecular( osg::Vec4( .1f, .3f, .1f, 1.f ));
		light->setPosition( osg::Vec4( -1.f, -3.f, 1.f, 0.f ));
	}

	osg::ref_ptr<osg::LightSource> ls = new osg::LightSource;
	ls->setLight( light.get() );
	//ls->setStateSetModes(*state, osg::StateAttribute::ON);
	root->addChild( ls.get() );
    }
}

void Vizkit3DWidget::addDataHandler(VizPluginBase *viz)
{
    root->addChild( viz->getVizNode() );
}

void Vizkit3DWidget::removeDataHandler(VizPluginBase *viz)
{
    root->removeChild( viz->getVizNode() );
}

void Vizkit3DWidget::changeCameraView(const osg::Vec3& lookAtPos)
{
    changeCameraView(&lookAtPos, 0, 0);
}

void Vizkit3DWidget::changeCameraView(const osg::Vec3& lookAtPos, const osg::Vec3& eyePos)
{
    changeCameraView(&lookAtPos, &eyePos, 0);
}

void Vizkit3DWidget::setCameraLookAt(double x, double y, double z)
{
    osg::Vec3 lookAt(x, y, z);
    changeCameraView(&lookAt, 0, 0);
}
void Vizkit3DWidget::setCameraEye(double x, double y, double z)
{
    osg::Vec3 eye(x, y, z);
    changeCameraView(0, &eye, 0);
}
void Vizkit3DWidget::setCameraUp(double x, double y, double z)
{
    osg::Vec3 up(x, y, z);
    changeCameraView(0, 0, &up);
}

void Vizkit3DWidget::changeCameraView(const osg::Vec3* lookAtPos, const osg::Vec3* eyePos, const osg::Vec3* upVector)
{
    osgGA::KeySwitchMatrixManipulator* switchMatrixManipulator = dynamic_cast<osgGA::KeySwitchMatrixManipulator*>(view->getCameraManipulator());
    if (!switchMatrixManipulator) return;
    //select TerrainManipulator
    switchMatrixManipulator->selectMatrixManipulator(3);
    
    //get last values of eye, center and up
    osg::Vec3d eye, center, up;
    switchMatrixManipulator->getHomePosition(eye, center, up);

    if (lookAtPos)
        center = *lookAtPos;
    if (eyePos)
        eye = *eyePos;
    if (upVector)
        up = *upVector;

    //set new values
    switchMatrixManipulator->setHomePosition(eye, center, up);
    view->home();
}

/**
 * Creates an instance of a visualization plugin using its
 * Vizkit Qt Plugin.
 * @param plugin Qt Plugin of the visualization plugin
 * @return Instance of the adapter collection of this plugin
 */
QObject* Vizkit3DWidget::createExternalPlugin(QObject* plugin, QString const& name)
{
    vizkit::VizkitQtPluginBase* qtPlugin = dynamic_cast<vizkit::VizkitQtPluginBase*>(plugin);
    if (qtPlugin) 
    {
        QStringList plugins = qtPlugin->getAvailablePlugins();
        vizkit::VizPluginBase* plugin = 0;
        if (name.isEmpty())
        {
            if (plugins.size() == 0)
            {
                std::cerr << "this Qt Designer plugin defines no vizkit plugins" << std::endl;
                return NULL;
            }
            else if (plugins.size() > 1)
            {
                std::cerr << "this Qt Designer plugin defines more than one vizkit plugin, you must select one explicitely" << std::endl;
                std::cerr << "available plugins are:" << std::endl;
                for (QStringList::const_iterator it = plugins.begin(); it != plugins.end(); ++it)
                    std::cerr << "  " << it->toStdString() << std::endl;
                return NULL;
            }

            plugin = qtPlugin->createPlugin(*plugins.begin());
        }
        else if (!plugins.contains(name))
        {
            if (plugins.contains(name + "Visualization"))
                plugin = qtPlugin->createPlugin(name + "Visualization");
            else
            {
                std::cerr << "there is no Vizkit plugin available called " << name.toStdString() << std::endl;
                std::cerr << "available plugins are:" << std::endl;
                for (QStringList::const_iterator it = plugins.begin(); it != plugins.end(); ++it)
                    std::cerr << "  " << it->toStdString() << std::endl;

                return NULL;
            }
        }
        else
        {
            plugin = qtPlugin->createPlugin(name);
        }

        if (!plugin)
        {
            std::cerr << "createPlugin returned NULL" << std::endl;
            return NULL;
        }
        addDataHandler(plugin);
        return plugin->getRubyAdapterCollection();
    }
    else 
    {
        std::cerr << "The given attribute is no Vizkit Qt Plugin!" << std::endl;
        return NULL;
    }
}

/**
 * Creates an instance of a visualization plugin given by its name 
 * and returns the adapter collection of the plugin, used in ruby.
 * @param pluginName Name of the plugin
 * @return Instance of the adapter collection of this plugin
 */
QObject* Vizkit3DWidget::createPluginByName(QString pluginName)
{
    vizkit::VizPluginBase* plugin = 0;
    if (pluginName == "WaypointVisualization")
    {
        plugin = new vizkit::WaypointVisualization();
    }
    else if (pluginName == "MotionCommandVisualization")
    {
        plugin = new vizkit::MotionCommandVisualization();
    }
    else if (pluginName == "TrajectoryVisualization")
    {
        plugin = new vizkit::TrajectoryVisualization();
    }
    else if (pluginName == "RigidBodyStateVisualization")
    {
        plugin = new vizkit::RigidBodyStateVisualization();
    }

    if (plugin) 
    {
        addDataHandler(plugin);
        VizPluginRubyAdapterCollection* adapterCollection = plugin->getRubyAdapterCollection();
        return adapterCollection;
    }
    else {
        std::cerr << "The Pluginname " << pluginName.toStdString() << " is unknown!" << std::endl;
        return NULL;
    }
}

/**
 * Returns a list of all available visualization plugins.
 * @return list of plugin names
 */
QStringList* Vizkit3DWidget::getListOfAvailablePlugins()
{
    if (!pluginNames->size()) 
    {
        pluginNames->push_back("WaypointVisualization");
        pluginNames->push_back("TrajectoryVisualization");
        pluginNames->push_back("MotionCommandVisualization");
        pluginNames->push_back("RigidBodyStateVisualization");
    }
    return pluginNames;
}