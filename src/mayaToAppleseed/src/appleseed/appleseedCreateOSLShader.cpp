#include "osl/oslUtils.h"
#include "utilities/logging.h"
#include "world.h"
#include "appleseed.h"
#include "renderer/modeling/shadergroup/shadergroup.h"
#include "OpenImageIO\typedesc.h"

MString oslTypeToMString(MAYATO_OSL::OSLParameter param)
{
	MString result;
	float vec[3];
	float m[4][4];
	void *val = nullptr;
	if (param.type == OSL::TypeDesc::TypeFloat)
	{
		result = "float ";
		result += boost::get<float>(param.value);
	}
	if (param.type == OSL::TypeDesc::TypeInt)
	{
		result = "int ";
		result += boost::get<int>(param.value);
	}
	if (param.type == OSL::TypeDesc::TypeVector)
	{
		MAYATO_OSL::SimpleVector &v = boost::get<MAYATO_OSL::SimpleVector>(param.value);
		result = MString("vector ") + v.f[0] + " " + v.f[1] + " " + v.f[2];
	}
	if (param.type == OSL::TypeDesc::TypeColor)
	{
		MAYATO_OSL::SimpleVector &v = boost::get<MAYATO_OSL::SimpleVector>(param.value);
		result = MString("color ") + v.f[0] + " " + v.f[1] + " " + v.f[2];
	}
	if (param.type == OSL::TypeDesc::TypeString)
	{
		result = MString("string ") + boost::get<std::string>(param.value).c_str();
	}
	if (param.type == OSL::TypeDesc::TypeMatrix)
	{
		MAYATO_OSL::SimpleMatrix &v = boost::get<MAYATO_OSL::SimpleMatrix>(param.value);
		result = MString("matrix ") + v.f[0][0] + " " + v.f[0][1] + " " + v.f[0][2] + " " + v.f[0][3] +
			v.f[1][0] + " " + v.f[1][1] + " " + v.f[1][2] + " " + v.f[1][3] +
			v.f[2][0] + " " + v.f[2][1] + " " + v.f[2][2] + " " + v.f[2][3] +
			v.f[3][0] + " " + v.f[3][1] + " " + v.f[3][2] + " " + v.f[3][3];
	}
	return result;
}

void MAYATO_OSL::createOSLShader(MString& shaderNodeType, MString& shaderName, OSLParamArray& paramArray, MString type)
{
	std::shared_ptr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;
	AppleRender::AppleseedRenderer *renderer = std::static_pointer_cast<AppleRender::AppleseedRenderer>(MayaTo::getWorldPtr()->worldRendererPtr).get();
	
	Logging::debug(MString("MAYATO_OSL::createOSLShader ") + shaderName);
	asr::ParamArray asParamArray;
	for (auto param : paramArray)
	{

		MString pname = param.name;
		if (pname == "color")
			pname = "inColor";

		MString paramString = oslTypeToMString(param);
		asParamArray.insert(pname.asChar(), paramString);
		Logging::debug(MString("\tParam ") + param.name + " " + paramString);
	}

	if (type == "shader")
	{
		Logging::debug(MString("MAYATO_OSL::createOSLShader creating shader node "));
		renderer->currentShaderGroup->add_shader("shader", shaderNodeType.asChar(), shaderName.asChar(), asParamArray);
	}
}

void MAYATO_OSL::connectOSLShaders(ConnectionArray& ca)
{
	std::shared_ptr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;
	AppleRender::AppleseedRenderer *renderer = std::static_pointer_cast<AppleRender::AppleseedRenderer>(MayaTo::getWorldPtr()->worldRendererPtr).get();
	for (auto connection : ca)
	{
		const char *srcLayer = connection.sourceNode.asChar();
		const char *srcAttr = connection.sourceAttribute.asChar();
		const char *destLayer = connection.destNode.asChar();
		MString destAttr = connection.destAttribute;
		if (destAttr == "color")
			destAttr = "inColor";
		Logging::debug(MString("MAYATO_OSL::connectOSLShaders ") + srcLayer + "." + srcAttr + " -> " + destLayer + "." + destAttr);
		renderer->currentShaderGroup->add_connection(srcLayer, srcAttr, destLayer, destAttr.asChar());
	}
}

