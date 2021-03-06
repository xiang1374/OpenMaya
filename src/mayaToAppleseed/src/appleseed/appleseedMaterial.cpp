#include "appleseed.h"
#include "appleseedUtils.h"
#include "shadingTools/material.h"
#include "shadingTools/shadingUtils.h"
#include "renderer/modeling/shadergroup/shadergroup.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrTools.h"
#include "OSL/oslUtils.h"
#include "maya/MFnDependencyNode.h"
#include "world.h"
#include "mayaScene.h"
#include "threads/renderQueueWorker.h"

void AppleRender::AppleseedRenderer::updateMaterial(MObject sufaceShader)
{

}


asf::StringArray AppleRender::AppleseedRenderer::defineMaterial(std::shared_ptr<mtap_MayaObject> obj)
{
	MStatus status;
	asf::StringArray materialNames;
	getObjectShadingGroups(obj->dagPath, obj->perFaceAssignments, obj->shadingGroups, false);
	std::shared_ptr<MayaScene> mayaScene = MayaTo::getWorldPtr()->worldScenePtr;

	for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
	{
		MAYATO_OSLUTIL::OSLUtilClass OSLShaderClass;
		MObject materialNode = obj->shadingGroups[sgId];
		MString shadingGroupName = getObjectName(materialNode);
		MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";
		MString surfaceShader;
		MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
		MString surfaceShaderName = getObjectName(surfaceShaderNode);
		ShadingNetwork network(surfaceShaderNode);
		size_t numNodes = network.shaderList.size();
		
		// if we are in IPR mode, save all translated shading nodes to the interactive update list
		if (MayaTo::getWorldPtr()->renderType == MayaTo::MayaToWorld::WorldRenderType::IPRRENDER)
		{
			if (mayaScene)
			{
				InteractiveElement iel;
				iel.mobj = surfaceShaderNode;
				iel.obj = obj;
				iel.name = surfaceShaderName;
				iel.node = surfaceShaderNode;
				mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;

				if (MayaTo::getWorldPtr()->renderState == MayaTo::MayaToWorld::WorldRenderState::RSTATERENDERING)
				{
					RenderQueueWorker::IPRUpdateCallbacks();
				}
			}
		}

		updateMaterial(surfaceShaderNode);

		
		asr::Assembly *assembly = getMasterAssemblyFromProject(this->project.get());
		assert(assembly != nullptr);
		asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

		if (shaderGroup != nullptr)
		{
			shaderGroup->clear();
		}
		else{
			asf::auto_release_ptr<asr::ShaderGroup> oslShadingGroup = asr::ShaderGroupFactory().create(shaderGroupName.asChar());
			assembly->shader_groups().insert(oslShadingGroup);
			shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());
		}

		OSLShaderClass.group = (OSL::ShaderGroup *)shaderGroup;

		MFnDependencyNode shadingGroupNode(materialNode);
		MPlug shaderPlug = shadingGroupNode.findPlug("surfaceShader");
		OSLShaderClass.createOSLProjectionNodes(shaderPlug);

		for (int shadingNodeId = 0; shadingNodeId < numNodes; shadingNodeId++)
		{
			ShadingNode snode = network.shaderList[shadingNodeId];
			Logging::debug(MString("ShadingNode Id: ") + shadingNodeId + " ShadingNode name: " + snode.fullName);
			if (shadingNodeId == (numNodes - 1))
				Logging::debug(MString("LastNode Surface Shader: ") + snode.fullName);
			OSLShaderClass.createOSLShadingNode(network.shaderList[shadingNodeId]);
			//OSLShaderClass.connectProjectionNodes(network.shaderList[shadingNodeId].mobject);
		}
			
		OSLShaderClass.cleanupShadingNodeList();
		OSLShaderClass.createAndConnectShaderNodes();
			
		//cleanupShadingNodelist - search for helper nodes and define them directly after the corresponding node
		//rename helper nodes with in/out prefix

		if (numNodes > 0)
		{
			ShadingNode snode = network.shaderList[numNodes - 1];
			MString layer = (snode.fullName + "_interface");
			Logging::debug(MString("Adding interface shader: ") + layer);
			asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
			sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
			const char *srcLayer = snode.fullName.asChar();
			const char *srcAttr = "outColor";
			const char *dstLayer = layer.asChar();
			const char *dstAttr = "inColor";
			Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
			sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
		}
			

		MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";
		assembly->surface_shaders().insert(
			asr::PhysicalSurfaceShaderFactory().create(
			physicalSurfaceName.asChar(),
			asr::ParamArray()));

		assembly->materials().insert(
			asr::OSLMaterialFactory().create(
			shadingGroupName.asChar(),
			asr::ParamArray()
			.insert("surface_shader", physicalSurfaceName.asChar())
			.insert("osl_surface", shaderGroupName.asChar())));

		MString objectInstanceName = getObjectInstanceName(obj.get());
		asr::Assembly *ass = getCreateObjectAssembly(obj);

		asr::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
		objInstance->get_front_material_mappings().insert("slot0", shadingGroupName.asChar());
	}

	return materialNames;

}
