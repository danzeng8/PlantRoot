﻿#include "BoostMetaGraph.h"
#include "boost/graph/connected_components.hpp"
#include "boost/graph/kruskal_min_spanning_tree.hpp"
#include <iostream>
#include <fstream>
#include <regex>

typedef int FaceI;

namespace
{
	GLfloat defaultColor[4] = { 1.0, 0.0, 0.0, 1.0 };

	struct sortEdge
	{
		GLuint v0, v1;
		sortEdge(GLuint f, GLuint s)
		{
			v0 = std::min(f, s);
			v1 = std::max(f, s);
		}

		bool operator<(sortEdge const &other)
		{
			if (v0 < other.v0)
			{
				return true;
			}
			else if (other.v0 < v0)
			{
				false;
			}
			else
			{
				return v1 < other.v1;
			}
		}

		bool operator==(sortEdge const &other)
		{
			return v0 == other.v0 && v1 == other.v1;
		}
	};
}

namespace Roots
{
	drawing::VBOSphere BMetaGraph::drawSphere = drawing::VBOSphere();
	drawing::VBOCube BMetaGraph::drawCube = drawing::VBOCube();
	float MinMaxStruct::minThickness = 10000;
	float MinMaxStruct::maxThickness = -1;

	float MinMaxStruct::minWidth = 10000;
	float MinMaxStruct::maxWidth = -1;

	float MinMaxStruct::minRatio = 10000;
	float MinMaxStruct::maxRatio = -1;

	float MinMaxStruct::minComponent = 10000;
	float MinMaxStruct::maxComponent = -1;

	float MinMaxStruct::minDegree = 0;
	float MinMaxStruct::maxDegree = -1;

	int BMetaEdge::instanceCounter = 0;

	std::vector<std::vector<float>> ColorTable::colors = {
			{0.902f, 0.098f, 0.294f, 1.0f},
			{0.235f, 0.706f, 0.294f, 1.0f},
			{1.0f, 0.882f, 0.098f, 1.0f},
			{0.0f, 0.51f, 0.784f, 1.0f},
			{0.961f, 0.51f, 0.188f, 1.0f},
			{0.569f, 0.118f, 0.706f, 1.0f},
			{0.275f, 0.941f, 0.941f, 1.0f},
			{0.941f, 0.196f, 0.902f, 1.0f},
			{0.824f, 0.961f, 0.235f, 1.0f},
			{0.98f, 0.745f, 0.745f, 1.0f},
			{0.0f, 0.502f, 0.502f, 1.0f},
			{0.902f, 0.745f, 1.0f, 1.0f},
			{0.667f, 0.431f, 0.157f, 1.0f},
			{1.0f, 0.98f, 0.784f, 1.0f},
			{0.502f, 0.0f, 0.0f, 1.0f},
			{0.667f, 1.0f, 0.765f, 1.0f},
			{0.502f, 0.502f, 0.0f, 1.0f},
			{1.0f, 0.843f, 0.706f, 1.0f},
			{0.0f, 0.0f, 0.502f, 1.0f},
			{0.502f, 0.502f, 0.502f, 1.0f},
			{0.902f, 0.098f, 0.294f, 1.0f},
			{0.235f, 0.706f, 0.294f, 1.0f},
			{1.0f, 0.882f, 0.098f, 1.0f},
			{0.0f, 0.51f, 0.784f, 1.0f},
			{0.961f, 0.51f, 0.188f, 1.0f},
			{0.569f, 0.118f, 0.706f, 1.0f},
			{0.275f, 0.941f, 0.941f, 1.0f},
			{0.941f, 0.196f, 0.902f, 1.0f},
			{0.824f, 0.961f, 0.235f, 1.0f},
			{0.98f, 0.745f, 0.745f, 1.0f},
			{0.0f, 0.502f, 0.502f, 1.0f},
			{0.902f, 0.745f, 1.0f, 1.0f},
			{0.667f, 0.431f, 0.157f, 1.0f},
			{1.0f, 0.98f, 0.784f, 1.0f},
			{0.502f, 0.0f, 0.0f, 1.0f},
			{0.667f, 1.0f, 0.765f, 1.0f},
			{0.502f, 0.502f, 0.0f, 1.0f},
			{1.0f, 0.843f, 0.706f, 1.0f},
			{0.0f, 0.0f, 0.502f, 1.0f},
			{0.502f, 0.502f, 0.502f, 1.0f}
	};

	/////////////////////////////////////////////// BMetaNode /////////////////////////////////////
	BMetaNode::BMetaNode()
	{
		mSrcVert = 0;
		mSrcSkeleton = nullptr;
		connectedComponent = -1;
		connectedPrimaryNode = -1;
		nodeThickness = 0.0;
		nodeWidth = 0.0;
		hasGeom = false;
		p[0] = 0;
		p[1] = 0;
		p[2] = 0;
		degree = 0;
		for (int i = 0; i < 4; ++i)
		{
			glThicknessColor[i] = defaultColor[i];
			glWidthColor[i] = defaultColor[i];
			glDegreeColor[i] = defaultColor[i];
			glComponentColor[i] = defaultColor[i];
			currentColor = glThicknessColor;
		}
	}
	BMetaNode::BMetaNode(SkelVert srcId, BSkeleton *skel)
	{
		mSrcVert = srcId;
		mSrcSkeleton = skel;
		connectedComponent = -1;
		connectedPrimaryNode = -1;
		nodeThickness = skel->operator[](srcId).thickness();
		nodeWidth = skel->operator[](srcId).width();
		hasGeom = true;
		p[0] = skel->operator[](srcId)[0];
		p[1] = skel->operator[](srcId)[1];
		p[2] = skel->operator[](srcId)[2];
		float degree = boost::degree(mSrcVert, *skel);

		MinMaxStruct::minDegree = std::min(degree, MinMaxStruct::minDegree);
		MinMaxStruct::maxDegree = std::max(degree, MinMaxStruct::maxDegree);

		MinMaxStruct::minThickness = std::min(skel->operator[](srcId).thickness(), MinMaxStruct::minThickness);
		MinMaxStruct::maxThickness = std::max(skel->operator[](srcId).thickness(), MinMaxStruct::maxThickness);

		MinMaxStruct::minWidth = std::min(skel->operator[](srcId).width(), MinMaxStruct::minWidth);
		MinMaxStruct::maxWidth = std::max(skel->operator[](srcId).width(), MinMaxStruct::maxWidth);

		float ratio = skel->operator[](srcId).thickness() / skel->operator[](srcId).width();

		MinMaxStruct::minRatio = std::min(ratio, MinMaxStruct::minRatio);
		MinMaxStruct::maxRatio = std::max(ratio, MinMaxStruct::maxRatio);


		for (int i = 0; i < 4; ++i)
		{
			glThicknessColor[i] = defaultColor[i];
			glWidthColor[i] = defaultColor[i];
			glDegreeColor[i] = defaultColor[i];
			glComponentColor[i] = defaultColor[i];
			currentColor = glThicknessColor;
		}
	}

	void BMetaNode::updateColors(NodeVisualizationOptions options, bool isUpdateComponentColor)
	{
		int length = options.heatmap.size() - 1;

		float thickRatio = ((nodeThickness - MinMaxStruct::minThickness) / (MinMaxStruct::maxThickness - MinMaxStruct::minThickness));
		float widthRatio = ((nodeWidth - MinMaxStruct::minWidth) / (MinMaxStruct::maxWidth - MinMaxStruct::minWidth));
		float degreeRatio = ((1.0*degree - MinMaxStruct::minDegree) / (MinMaxStruct::maxDegree - MinMaxStruct::minDegree));

		thickRatio = (thickRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);
		widthRatio = (widthRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);
		degreeRatio = (degreeRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);

		thickRatio = std::min(thickRatio, 1.0f);
		thickRatio = std::max(thickRatio, 0.0f);
		widthRatio = std::min(widthRatio, 1.0f);
		widthRatio = std::max(widthRatio, 0.0f);
		degreeRatio = std::min(degreeRatio, 1.0f);
		degreeRatio = std::max(degreeRatio, 0.0f);

		int thickpos = thickRatio * length;
		int widthpos = widthRatio * length;
		int degreepos = degreeRatio * length;

		thickpos = std::min(thickpos, length);
		widthpos = std::min(widthpos, length);
		degreepos = std::min(degreepos, length);

		thickpos = std::max(thickpos, 0);
		widthpos = std::max(widthpos, 0);
		degreepos = std::max(degreepos, 0);


		if (thickpos > length || widthpos > length || degreepos > length)
		{
			std::cout << "Value out of range : thickpos " << thickpos << " width pos : " << widthpos << " ratiopos : " << degreepos << std::endl;
		}
		int p = 3;


		std::vector<GLfloat> thicknessColor = options.heatmap[thickpos];
		std::vector<GLfloat> widthColor = options.heatmap[widthpos];
		std::vector<GLfloat> degreeColor = options.heatmap[degreepos];
		for (int c = 0; c < 3; ++c)
		{
			glThicknessColor[c] = thicknessColor[c];

			glWidthColor[c] = widthColor[c];

			glDegreeColor[c] = degreeColor[c];
		}
		currentColor = glThicknessColor;
		if (isUpdateComponentColor)
		{
			updateComponentColor();
		}
	}

	void BMetaNode::updateComponentColor()
	{

		std::vector<float> componentColor = ColorTable::getComponentColor(connectedComponent);
		for (int c = 0; c < 3; ++c)
		{
			glComponentColor[c] = componentColor[c];
		}
	}

	////////////////////////////////////////////// BMetaEdge ///////////////////////////////////////

	BMetaEdge::BMetaEdge()
	{
		mVertices = {};
		mEdges = {};
		indicesList = {};
		localThicknessColors = {};
		localWidthColors = {};
		localRatioColors = {};
		localComponentColors = {};
		averageThickness = 0.0;
		averageWidth = 0.0;
		mLength = 0.0;
		isBridge = false;
		isSelected = false;
		connectedComponent = -1;
		connectedPrimaryNode = -1;
		instanceId = instanceCounter;
		++instanceCounter;
	}

	BMetaEdge::BMetaEdge(std::vector<SkelVert> vertices, BSkeleton* srcSkeleton)
	{
		mVertices = vertices;
		mEdges = {};
		indicesList = {};
		localThicknessColors = {};
		localWidthColors = {};
		localRatioColors = {};
		localComponentColors = {};
		float weightedAvgThickness = 0.0;
		float weightedAvgWidth = 0.0;
		float skelEdgeLength = 0.0;
		averageThickness = 0.0;
		averageWidth = 0.0;
		mLength = 0.0;
		isBridge = false;
		isSelected = false;
		connectedComponent = -1;
		connectedPrimaryNode = -1;
		instanceId = instanceCounter;
		++instanceCounter;
		for (int i = 0; i < mVertices.size() - 1; ++i)
		{
			SkelVert v0 = mVertices[i];
			SkelVert v1 = mVertices[i + 1];
			SkelEdge e;
			bool exists;
			boost::tie(e, exists) = boost::edge(v0, v1, *srcSkeleton);
			if (exists)
			{
				RootAttributes skelRootAttributes = srcSkeleton->operator[](e);
				Point3d *p0 = &srcSkeleton->operator[](v0);
				mEdges.push_back(skelRootAttributes);
				float length = skelRootAttributes.euclidLength;
				weightedAvgThickness += p0->thickness() * length;
				weightedAvgWidth += p0->width() * length;
				skelEdgeLength += length;
				indicesList.push_back(v0);
				indicesList.push_back(v1);
			}
			else
			{
				std::cout << "Attempting to create a metaedge over skeleton vertices that are not already an edge" << std::endl;
				exit(1);
			}
		}
		if (skelEdgeLength == 0)
		{
			skelEdgeLength = 1;
		}
		averageThickness = weightedAvgThickness / skelEdgeLength;

		if (isnan(averageThickness))
		{
			averageThickness = 1.0;
			std::cout << "Note : improper MetaEdge thickness, skeleton data may be missing thickness data on one or more roots" << std::endl;
		}

		averageWidth = weightedAvgWidth / skelEdgeLength;

		if (isnan(averageWidth))
		{
			averageWidth = 1.0;
			std::cout << "Note : improper MetaEdge width, skeleton data may be missing width data on one or more roots" << std::endl;
		}

		mLength = skelEdgeLength;
		for (int i = 0; i < mVertices.size() - 1; ++i)
		{
			for (int goTwice = 0; goTwice < 2; ++goTwice)
			{
				for (int c = 0; c < 3; ++c)
				{
					localThicknessColors.push_back(defaultColor[c]);
					localWidthColors.push_back(defaultColor[c]);
					localRatioColors.push_back(defaultColor[c]);
					localComponentColors.push_back(defaultColor[c]);
				}
			}
		}
	}

	void BMetaEdge::addToIndicesList(std::vector<GLuint> &edgeIndices)
	{
		edgeIndices.insert(edgeIndices.end(), indicesList.begin(), indicesList.end());
	}

	BMetaEdge BMetaEdge::join(BMetaEdge &other, BSkeleton *srcSkeleton)
	{
		int myMaxId = mVertices.size() - 1;
		int otherMaxId = other.mVertices.size() - 1;
		std::vector<SkelVert> joinedVertices = {};
		if (mVertices[myMaxId] != other.mVertices[0] &&
			mVertices[myMaxId] != other.mVertices[otherMaxId] &&
			mVertices[0] != other.mVertices[0] &&
			mVertices[0] != other.mVertices[otherMaxId])
		{
			return BMetaEdge();
		}
		//in the case that the last vertex in this edge is the starting vertex in the 
		//other edge, simply append all but the first vertex of that edge to the vertex list
		//of this
		if (mVertices[myMaxId] == other.mVertices[0])
		{
			for (int i = 0; i < mVertices.size(); ++i)
			{
				joinedVertices.push_back(mVertices[i]);
			}
			for (int i = 1; i < other.mVertices.size(); ++i)
			{
				joinedVertices.push_back(other.mVertices[i]);
			}
		}
		else if (mVertices[myMaxId] == other.mVertices[otherMaxId])
		{
			for (int i = 0; i < mVertices.size(); ++i)
			{
				joinedVertices.push_back(mVertices[i]);
			}
			for (int i = otherMaxId - 1; i >= 0; --i)
			{
				joinedVertices.push_back(other.mVertices[i]);
			}
		}
		else if (mVertices[0] == other.mVertices[0])
		{
			for (int i = myMaxId; i >= 0; --i)
			{
				joinedVertices.push_back(mVertices[i]);
			}
			for (int i = 1; i <= otherMaxId; ++i)
			{
				joinedVertices.push_back(other.mVertices[i]);
			}
		}
		else
		{
			//to avoid repetitive code, simply flip the of the ordering for the same result
			//if matches are not made on this end.
			return other.join(*this, srcSkeleton);
		}

		BMetaEdge result = BMetaEdge(joinedVertices, srcSkeleton);
		result.connectedComponent = other.connectedComponent;
		return result;

	}

	std::pair<BMetaEdge, BMetaEdge> BMetaEdge::split(SkelVert toSplitOn, BSkeleton *srcSkeleton)
	{
		std::vector<SkelVert> leftEdge = {}, rightEdge = {};
		std::vector<GLuint> leftEdgeIndices = {}, rightEdgeIndices = {};
		bool onLeft = true;
		for (int i = 0; i < mVertices.size(); ++i)
		{

			if (mVertices[i] == toSplitOn)
			{
				onLeft = false;
				rightEdge.push_back(mVertices[i]);
				leftEdge.push_back(mVertices[i]);
				continue;
			}
			if (onLeft)
			{
				leftEdge.push_back(mVertices[i]);
			}
			if (!onLeft)
			{
				rightEdge.push_back(mVertices[i]);
			}
		}
		std::cout << std::endl << "Left edge verts";
		for (SkelVert v : leftEdge)
		{
			std::cout << " " << v;
		}
		std::cout << std::endl << "left edge indices";
		for (GLuint idx : leftEdgeIndices)
		{
			std::cout << " " << idx;
		}
		std::cout << std::endl << "right edge verts";
		for (SkelVert v : rightEdge)
		{
			std::cout << " " << v;
		}
		std::cout << std::endl << "right edge indices";
		for (GLuint idx : rightEdgeIndices)
		{
			std::cout << " " << idx;
		}

		BMetaEdge left = BMetaEdge(leftEdge, srcSkeleton);
		BMetaEdge right = BMetaEdge(rightEdge, srcSkeleton);

		return std::make_pair(left, right);
	}

	float BMetaEdge::getAvgThickness()
	{
		return averageThickness;
	}

	SkelVert BMetaEdge::start()
	{
		return mVertices[0];
	}
	SkelVert BMetaEdge::end()
	{
		return mVertices[mVertices.size() - 1];
	}

	void BMetaEdge::updateColors(EdgeVisualizationOptions options, std::vector<std::vector<GLfloat>> &vertexColors, BSkeleton *srcSkeleton)
	{
		int length = options.heatmap.size() - 1;

		localThicknessColors.resize(mVertices.size() * 3);
		localWidthColors.resize(mVertices.size() * 3);
		localRatioColors.resize(mVertices.size() * 3);
		localComponentColors.resize(mVertices.size() * 3);

		for (int i = 0; i < mVertices.size(); ++i)
		{
			Point3d p = srcSkeleton->operator[](mVertices[i]);

			float thickRatio = ((p.thickness() - MinMaxStruct::minThickness) / (MinMaxStruct::maxThickness - MinMaxStruct::minThickness));
			float widthRatio = ((p.width() - MinMaxStruct::minWidth) / (MinMaxStruct::maxWidth - MinMaxStruct::minWidth));
			float ratio = p.ratio();
			float ratioRatio = ((ratio - MinMaxStruct::minRatio) / (MinMaxStruct::maxRatio - MinMaxStruct::minRatio));


			thickRatio = (thickRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);
			widthRatio = (widthRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);
			ratioRatio = 1.0 - (ratioRatio - options.minColorCutoff) / (options.maxColorCutoff - options.minColorCutoff);

			thickRatio = std::min(1.0f, thickRatio);
			thickRatio = std::max(0.0f, thickRatio);

			widthRatio = std::min(1.0f, widthRatio);
			widthRatio = std::max(0.0f, widthRatio);

			ratioRatio = std::min(1.0f, ratioRatio);
			ratioRatio = std::max(0.0f, ratioRatio);


			int thickpos = thickRatio * length;
			int widthpos = widthRatio * length;
			int ratiopos = ratioRatio * length;

			if (thickpos > length || widthpos > length || ratiopos > length)
			{
				std::cout << "Value out of range : thickpos " << thickpos << " width pos : " << widthpos << " ratiopos : " << ratiopos << std::endl;
			}

			thickpos = std::min(thickpos, length);
			widthpos = std::min(widthpos, length);
			ratiopos = std::min(ratiopos, length);
			thickpos = std::max(thickpos, 0);
			widthpos = std::max(widthpos, 0);
			ratiopos = std::max(ratiopos, 0);

			std::vector<GLfloat> thicknessColor = options.heatmap[thickpos];
			std::vector<GLfloat> widthColor = options.heatmap[widthpos];
			std::vector<GLfloat> ratioColor = options.heatmap[ratiopos];

			for (int c = 0; c < 3; ++c)
			{
				localThicknessColors[i * 3 + c] = thicknessColor[c];
				localWidthColors[i * 3 + c] = widthColor[c];
				localRatioColors[i * 3 + c] = ratioColor[c];
			}
		}

		updateComponentColor(vertexColors);

		updateGraphColors(vertexColors);
	}

	void BMetaEdge::updateComponentColor(std::vector<std::vector<GLfloat>> &vertexColors)
	{

		std::vector<float> componentColor = ColorTable::getComponentColor(connectedComponent);

		for (int i = 0; i < mVertices.size(); ++i)
		{
			for (int c = 0; c < 3; ++c)
			{
				localComponentColors[i * 3 + c] = componentColor[c];
			}
		}
		updateGraphColors(vertexColors);
	}

	void BMetaEdge::select(GLfloat *selectionColor, std::vector<std::vector<GLfloat>> &vertexColors)
	{
		isSelected = true;
		std::cout << "Calling select" << std::endl;
		std::cout << "edge count " << mEdges.size() << std::endl;
		for (int i = 0; i < mVertices.size(); ++i)
		{
			for (int coloringType = 0; coloringType < 4; ++coloringType)
			{
				for (int c = 0; c < 3; ++c)
				{
					vertexColors[coloringType][mVertices[i] * 3 + c] = selectionColor[c];
				}
			}
		}
		std::cout << "ending select" << std::endl;
	}

	void BMetaEdge::unselect(std::vector<std::vector<GLfloat>> &vertexColors)
	{
		isSelected = false;
		updateGraphColors(vertexColors);
	}

	void BMetaEdge::highLight(GLfloat *selectionColor, std::vector<std::vector<GLfloat>> &vertexColors)
	{
		for (int i = 0; i < mVertices.size(); ++i)
		{
			for (int coloringType = 0; coloringType < 4; ++coloringType)
			{
				for (int c = 0; c < 3; ++c)
				{
					vertexColors[coloringType][mVertices[i] * 3 + c] = selectionColor[c];
				}
			}
		}
	}

	void BMetaEdge::unhighLigh(std::vector<std::vector<GLfloat>> &vertexColors)
	{
		updateGraphColors(vertexColors);
	}

	void BMetaEdge::updateGraphColors(std::vector<std::vector<GLfloat>> &vertexColors)
	{
		int maxIndex = 0;
		for (int i = 0; i < mVertices.size(); ++i)
		{
			maxIndex = std::max(maxIndex, (int)mVertices[i]);
		}
		if (maxIndex * 3 + 3 > vertexColors[0].size())
		{
			for (int i = 0; i < 4; ++i)
			{
				vertexColors[i].resize(maxIndex * 3 + 3);
			}
		}

		for (int i = 0; i < mVertices.size(); ++i)
		{
			for (int c = 0; c < 3; ++c)
			{
				vertexColors[0][mVertices[i] * 3 + c] = localThicknessColors[i * 3 + c];
				vertexColors[1][mVertices[i] * 3 + c] = localWidthColors[i * 3 + c];
				vertexColors[2][mVertices[i] * 3 + c] = localRatioColors[i * 3 + c];
				vertexColors[3][mVertices[i] * 3 + c] = localComponentColors[i * 3 + c];
			}
		}
	}

	MetaFace::MetaFace(std::set<FaceI> memberFaces, std::vector<Face>& skelFaces)
	{
		faceIndices = memberFaces;
		center = Point3d();
		vertices = {};
		vertices.resize(memberFaces.size() * 3);
		int face = 0;
		int i = 0;
		for (std::set<FaceI>::iterator iter = memberFaces.begin(); iter != memberFaces.end(); ++iter, ++i)
		{
			face = *iter;
			center += skelFaces[face].center;

			for (int v = 0; v < 3; ++v)
			{
				vertices[i * 3 + v] = skelFaces[face].vertices[v];
			}
		}

		for (int dim = 0; dim < 3; ++dim)
		{
			center[dim] /= memberFaces.size();
		}
	}


	void generateMetaFace(std::vector<Face> &allFaces, std::vector<std::vector<FaceI>> &matchingPairs, std::vector<std::vector<bool>> &visited, int &startI, std::set<FaceI> &result)
	{
		if (startI >= allFaces.size())
		{
			return;
		}
		for (int k = 0; k < visited[startI].size(); ++k)
		{
			if (!visited[startI][k])
			{
				result.insert(matchingPairs[startI][k]);
				visited[startI][k] = true;
				generateMetaFace(allFaces, matchingPairs, visited, matchingPairs[startI][k], result);
			}
		}
		return;
	}

	std::vector<MetaFace> MetaFace::findMetaFaces(std::vector<Face> &allFaces)
	{
		//vector which for each face indicates its neighbors (share an edge)
		std::vector<std::vector<FaceI>> faceNeighbors = std::vector<std::vector<FaceI>>(allFaces.size(), std::vector<FaceI>());

		std::vector<MetaFace> result = {};

		for (FaceI i = 0; i < allFaces.size(); ++i)
		{
			Face face1 = allFaces[i];
			for (FaceI k = i + 1; k < allFaces.size(); ++k)
			{
				Face face2 = allFaces[k];
				int matchedVerts = 0;
				for (int v1 = 0; v1 < 3; ++v1)
				{
					for (int v2 = 0; v2 < 3; ++v2)
					{
						if (face1.vertices[v1] == face2.vertices[v2])
						{
							matchedVerts++;
						}
					}
				}
				if (matchedVerts >= 2)
				{
					faceNeighbors[i].push_back(k);
					faceNeighbors[k].push_back(i);
				}
			}
		}

		std::vector<std::vector<bool>> visited = {};

		for (FaceI i = 0; i < faceNeighbors.size(); ++i)
		{
			std::vector<bool> visitedLine = {};
			for (FaceI k = 0; k < faceNeighbors[i].size(); ++k)
			{
				visitedLine.push_back(false);
			}
			visited.push_back(visitedLine);
		}

		for (FaceI startI = 0; startI < faceNeighbors.size(); ++startI)
		{
			std::set<FaceI> metaFaceIs = {};
			generateMetaFace(allFaces, faceNeighbors, visited, startI, metaFaceIs);

			if (metaFaceIs.size() > 0)
			{
				result.push_back(MetaFace(metaFaceIs, allFaces));
			}
		}

		return result;
	}


	///////////////////////////////////////// BMetaGraph //////////////////////////////////////////

	BMetaGraph::BMetaGraph()
	{
		std::cout << "Called: Constructor BMetaGraph" << std::endl;
		Initialize();
	}

	void BMetaGraph::Initialize()
	{
		std::cout << "Called: Initialize" << std::endl;
		mSkeleton = BSkeleton();
		vertNodeMap = std::map<SkelVert, MetaV>();
		mComponentMap = {};
		mComponentSizeMap = {};
		mMinimumSpanningTree = {};
		numComponents = -1;
		mDuplicateEdgeMap = std::map<MetaE, MetaE>();
		mDuplicateNodeMap = std::map<MetaV, MetaV>();
		//componentBounds = {};
		isLoaded = false;

		displayFaces = true;
		prevShowEndpoints = false;
		prevShowJunctions = false;
		edgeOptions = EdgeVisualizationOptions();
		nodeOptions = NodeVisualizationOptions();
		mode = OperationMode::None;


		vertexColors = { {},{},{},{} };
		selectionVBO = {};
		bridgeVBO = {};
		nonBridgeVBO = {};
		selectedSegmentVBO = {};
		primaryBranchesVBO = {};
		stemVBO = {};

		autoStemVBO = {};
		traceBranchVBO = {};

		// 1. whorl edges on stem
		// 2. non whorl edges on stem
		// 3. branches associated with current selected whorl
		// 4. branches not associated with current selected whorl
		// 5. other edges (not stem, not traced branches)

		whorlEdgeStemVBO = {};
		nonWhorlEdgeStemVBO = {};
		selectedWhorlBranchesVBO = {};
		nonSelectedWhorlBranchesVBO = {};
		nonStemNonBranchesVBO = {};

		testVBO = {};


		for (int i = 0; i < 3; ++i)
		{
			selectionColor[i] = 0.0;
		}
		selectionColor[3] = 1.0;
		selectionColor[2] = 1.0;
		eyeShiftX = 0;
		eyeShiftY = 0;
		viewCenter = Point3d();


		alphaMesh = Mesh();
		displayMesh = false;

		onlyDisplaySelectedComponents = false;
		selectedComponent1 = 0, selectedComponent2 = 0;
		showBoundingBoxes = false;

		selectNode1Valid = false;
		selectNode2Valid = false;
		selectNode1 = 99999999;
		selectNode2 = 99999999;

		breakEdgeValid = false;

		splitEdgeValid = false;
		splitNeighbors = {};

		removeComponentEdgeValid = false;

		InitializeTraitParameters();

		clear();
		arcball_reset();

		nodeToView = 99999999;
		viewNodeInfoValid = false;
	}

	void BMetaGraph::InitializeTraitParameters()
	{
		std::cout << "Called: InitializeTraitParameters" << std::endl;
		showSuggestedStem = false;
		showStem = false;
		showTracedBranches = false;
		tracePrimaryBranches = false;
		selectStemStartValid = false;
		selectStemEndValid = false;
		selectStemStart = 99999999;
		selectStemEnd = 99999999;
		//StemPath = {};
		StemPath_node = {};
		stemSelected = false;
		auto_stem = {};

		showWhorl = false;
		showClusterInput = false;
		selectWhorlValid = false;
		selectedWhorl = 99999999;
		whorls = {};
		auto_node = {};
		invalidWhorlCandidates = {};
		boundNodeCandidates = {};
		showPrimaryNodes = false;
		PrimaryNodes = {};
		selectPrimaryNodesValid = false;
		NodesPredecessors = {};

		CurrentPrimaryNode = -1;
		CurrentPredecessors = {};
		PrimaryBranchSelectionValid = false;
		PrimaryBranchSelection = 99999999;
		PrimaryBranchesObj = {};
		showConfirmedPrimaryBranches = false;
		showTraitsOnly = false;

		isTraitsLoad = false;
		isRandomColorizePrimaryNodes = false;
		showOnlyBranchesOfCurrentPrimaryNode = false;
		showSelectedSegment = false;
		TraitsNodes = {};

		selectSegmentPoint1 = 99999999;
		selectSegmentPoint2 = 99999999;
		selectSegmentPoint1Valid = false;
		selectSegmentPoint2Valid = false;
		SegmentMetaEdges = {};
		SegmentMetaNodes = {};
		SegmentHorizontalRadius = 20;
		SegmentPath = {};
		SegmentPath_node = {};
		SegmentNodesDistances = {};

	}

	void BMetaGraph::loadFromFile(std::string filename)
	{
		std::cout << "Called: loadFromFile" << std::endl;
		Initialize();

		if (filename.length() == 0)
		{
			std::cout << "Provided filename is empty" << std::endl;
			return;
		}

		skeleton_file_name = filename;
		std::ifstream filestream;
		filestream.open(filename);
		std::vector<std::string> lines = {};
		std::string line;
		while (!filestream.eof())
		{
			std::getline(filestream, line);
			lines.push_back(line);
		}
		filestream.close();
		loadFromLines(lines, 0);
		findAndLabelConnectedComponents();
		isLoaded = true;
		if (alphaMesh.vertices.size() != 0)
		{
			alphaMesh.recenter((mSkeleton.mCenter - mSkeleton.originalCenter).p);
		}
	}

	void BMetaGraph::loadMeshFromFile(std::string filename)
	{
		alphaMesh.loadFromOff(filename, (mSkeleton.mCenter - mSkeleton.originalCenter).p);
	}

	int BMetaGraph::loadFromLines(std::vector<std::string> &lines, int startingLine)
	{
		std::cout << "Called: LoadFromLines" << std::endl;
		int result = 0;
		int lastLine = loadSkeletonFromLines(lines, startingLine); // load skeleton header and content
		mSkeleton.findBoundingSphere();
		if (useArcball)
		{
			viewCenter = mSkeleton.mCenter;
		}

		SkelVertIterator svi = boost::vertices(mSkeleton);
		float thickness = 0.00001, width = 0.00001, ratio = 0;
		for (; svi.first != svi.second; ++svi)
		{
			Point3d *p = &mSkeleton[*svi.first];
			thickness = p->thickness();
			width = p->width();
			ratio = p->ratio();

			MinMaxStruct::minThickness = std::min(thickness, MinMaxStruct::minThickness);
			MinMaxStruct::maxThickness = std::max(thickness, MinMaxStruct::maxThickness);

			MinMaxStruct::minWidth = std::min(width, MinMaxStruct::minWidth);
			MinMaxStruct::maxWidth = std::max(width, MinMaxStruct::maxWidth);

			MinMaxStruct::minRatio = std::min(ratio, MinMaxStruct::minRatio);
			MinMaxStruct::maxRatio = std::max(ratio, MinMaxStruct::maxRatio);
		}

		bool fileHasMetaInfo = checkHeaderForMetaInfo(lines);
		std::cout << "file has Meta info? " << fileHasMetaInfo << std::endl;
		if (fileHasMetaInfo)
		{
			result = loadGraphFromLines(lines, lastLine);
		}
		else
		{
			initializeFromSkeleton();
		}
		findBridges();
		std::cout << "Building edge vbos " << std::endl;
		buildEdgeVBOs();
		std::cout << "Finished building edge vbos" << std::endl;
		return result;
	}

	void BMetaGraph::writeToStream(std::ostream & out)
	{
		mSkeleton.writeToStream(out);
	}

	void BMetaGraph::saveToFile(std::string filename)
	{
		std::ofstream filestream;
		filestream.open(filename);

		filestream << "ply" << std::endl;
		filestream << "format ascii 1.0" << std::endl;
		filestream << "element vertex " << mSkeleton.m_vertices.size() << std::endl;
		for (int i = 0; i < ParsingOrder::ParsingCount; ++i)
		{
			if (mSkeleton.mVertexWriteOrder[i] == ParsingOrder::X)
			{
				filestream << "property float32 x" << std::endl;
			}
			if (mSkeleton.mVertexWriteOrder[i] == ParsingOrder::Y)
			{
				filestream << "property float32 y" << std::endl;
			}
			if (mSkeleton.mVertexWriteOrder[i] == ParsingOrder::Z)
			{
				filestream << "property float32 z" << std::endl;
			}
			if (mSkeleton.mVertexWriteOrder[i] == ParsingOrder::Thickness)
			{
				filestream << "property float32 bt2" << std::endl;
			}
			if (mSkeleton.mVertexWriteOrder[i] == ParsingOrder::Width)
			{
				filestream << "property float32 radius" << std::endl;
			}
		}
		filestream << "element edge " << mSkeleton.m_edges.size() << std::endl;
		filestream << "property int32 vertex1" << std::endl;
		filestream << "property int32 vertex2" << std::endl;
		filestream << "element face " << mSkeleton.faces.size() << std::endl;
		filestream << "property list uint8 int32 vertex_indices" << std::endl;
		filestream << "element MetaNode " << m_vertices.size() << std::endl;
		filestream << "property int32 srcVert " << std::endl;
		filestream << "element MetaEdge " << m_edges.size() << std::endl;
		filestream << "property int32 node1" << std::endl;
		filestream << "property int32 node2" << std::endl;
		filestream << "property list int32 int32 node_indices" << std::endl;
		filestream << "element MetaFace " << faces.size() << std::endl;
		filestream << "property list int32 int32 face_indices" << std::endl;
		filestream << "end_header" << std::endl;
		mSkeleton.writeDanPly(filestream);

		metaVertIter mvi = boost::vertices(*this);
		for (; mvi.first != mvi.second; ++mvi)
		{
			filestream << operator[](*mvi.first).mSrcVert << std::endl;
		}

		metaEdgeIter mei = boost::edges(*this);
		for (; mei.first != mei.second; ++mei)
		{
			BMetaEdge *e = &operator[](*mei.first);
			filestream << mei.first->m_source << " " << mei.first->m_target << " " << e->mVertices.size();
			for (int i = 0; i < e->mVertices.size(); ++i)
			{
				filestream << " " << e->mVertices[i];
			}
			filestream << std::endl;
		}

		for (int i = 0; i < faces.size(); ++i)
		{
			filestream << faces[i].faceIndices.size();
			for (std::set<FaceI>::iterator iter = faces[i].faceIndices.begin(); iter != faces[i].faceIndices.end(); ++iter)
			{
				filestream << " " << *iter;
			}
			filestream << std::endl;
		}

	}

	void BMetaGraph::saveFairedSkeleton(std::string filename) {
		if (mSkeleton.m_edges.size() == 0) {
			std::cout << "Error: No skeleton found." << std::endl;
			return;
		}
		BSkeleton fairedSkeleton = fairSkeleton(mSkeleton, 10);
		std::cout << std::endl;
		std::ofstream filestream;
		filestream.open(filename);
		filestream << "ply" << std::endl;
		filestream << "format ascii 1.0" << std::endl;
		filestream << "element vertex " << fairedSkeleton.m_vertices.size() << std::endl;
		for (int i = 0; i < ParsingOrder::ParsingCount; ++i)
		{
			if (fairedSkeleton.mVertexWriteOrder[i] == ParsingOrder::X)
			{
				filestream << "property float32 x" << std::endl;
			}
			if (fairedSkeleton.mVertexWriteOrder[i] == ParsingOrder::Y)
			{
				filestream << "property float32 y" << std::endl;
			}
			if (fairedSkeleton.mVertexWriteOrder[i] == ParsingOrder::Z)
			{
				filestream << "property float32 z" << std::endl;
			}
			if (fairedSkeleton.mVertexWriteOrder[i] == ParsingOrder::Thickness)
			{
				filestream << "property float32 bt2" << std::endl;
			}
			if (fairedSkeleton.mVertexWriteOrder[i] == ParsingOrder::Width)
			{
				filestream << "property float32 radius" << std::endl;
			}
		}
		filestream << "element edge " << fairedSkeleton.m_edges.size() << std::endl;
		filestream << "property int32 vertex1" << std::endl;
		filestream << "property int32 vertex2" << std::endl;
		filestream << "element face " << fairedSkeleton.faces.size() << std::endl;
		filestream << "property list uint8 int32 vertex_indices" << std::endl;
		filestream << "end_header" << std::endl;
		SkelVertIterator svi = boost::vertices(fairedSkeleton);
		for (; svi.first != svi.second; ++svi) {
			Point3d point = fairedSkeleton.operator[](*svi.first);
			filestream << point.thickness() << " " << point.width() << " " << point.x() << " " << point.y() << " " << point.z() << std::endl;
		}
		skelEdgeIter sei = boost::edges(fairedSkeleton);
		for (; sei.first != sei.second; ++sei) {
			SkelEdge se = *sei.first;
			filestream << se.m_source << " " << se.m_target << std::endl;
		}
		filestream.close();
		std::cout << "File saved: " << filename << std::endl;
	}

	void BMetaGraph::loadTraitsFromFile(std::string filename)
	{
		InitializeTraitParameters();

		if (filename.length() == 0)
		{
			std::cout << "Provided filename is empty" << std::endl;
			return;
		}

		std::ifstream filestream;
		filestream.open(filename);
		std::vector<std::string> lines = {};
		std::string line;
		while (!filestream.eof())
		{
			std::getline(filestream, line);
			lines.push_back(line);
		}
		filestream.close();
		loadTraitsFromLines(lines, 0);
		isTraitsLoad = true;
	}

	int BMetaGraph::loadTraitsFromLines(std::vector<std::string> &lines, int startingLine)
	{
		bool endHeaderReached = false;
		std::string line;
		int numEdges = 0, numVerts = 0;
		line = lines[startingLine];
		boost::trim(line);

		int result = startingLine;
		std::cout << "Traits file type: " << line << std::endl;
		if (boost::iequals(line, "ply"))
		{
			result = loadTraitsPly(lines, startingLine);
		}
		else
		{
			std::cout << "This is recognized as a PLY style file...." << std::endl;
		}

		buildEdgeVBOs();
		std::cout << "Finished building edge vbos" << std::endl;
		return result;
	}

	int BMetaGraph::loadTraitsPly(std::vector<std::string> &lines, int startingLine)
	{
		int lineOn = startingLine;

		int numPrimaryNodes = 0, numPrimaryBranches = 0;
		std::vector<std::string> words = {};

		bool headerEnded = false;

		while (!headerEnded)
		{
			std::cout << lineOn << std::endl;
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
			//vertices
			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "stem")) {}
			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "primary_nodes"))
			{
				numPrimaryNodes = boost::lexical_cast<int>(words[2]);
			}
			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "primary_branches"))
			{
				numPrimaryBranches = boost::lexical_cast<int>(words[2]);
			}
			if (boost::iequals(words[0], "end_header"))
			{
				headerEnded = true;
			}
			++lineOn;
		}
		std::cout << "loading stem " << std::endl;
		loadStem(lines, lineOn);
		std::cout << "loading primary nodes " << std::endl;
		loadPrimaryNodes(lines, lineOn, numPrimaryNodes);
		std::cout << "loading primary branches " << std::endl;
		loadPrimaryBranches(lines, lineOn, numPrimaryBranches);
		std::cout << "Finished loading skeleton " << std::endl;

		return lineOn;
	}

	void BMetaGraph::loadStem(std::vector<std::string> &lines, int &lineOn)
	{
		std::cout << "loadStem" << std::endl;
		std::vector<std::string> words = {};
		words = {};

		boost::split(words, lines[lineOn], boost::is_any_of(" "));
		selectStemStart = boost::lexical_cast<unsigned int>(words[0]);
		++lineOn;

		boost::split(words, lines[lineOn], boost::is_any_of(" "));
		selectStemEnd = boost::lexical_cast<unsigned int>(words[0]);
		++lineOn;

		selectStemStartValid = true;
		selectStemEndValid = true;
		SelectStemOperation();
	}

	void BMetaGraph::loadPrimaryNodes(std::vector<std::string> &lines, int &lineOn, int numPrimaryNodes)
	{
		std::vector<std::string> words;

		for (int i = 0; i < numPrimaryNodes; ++i, ++lineOn)
		{
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
			int index = std::atoi(words[0].c_str());
			MetaV node = boost::lexical_cast<unsigned int>(words[1]);
			PrimaryNodes.push_back(std::make_pair(index, node));
		}

		SelectStemPrimaryNodeOperation();
	}

	void BMetaGraph::loadPrimaryBranches(std::vector<std::string> &lines, int &lineOn, int numPrimaryBranches)
	{
		std::vector<std::string> words;
		int wordOn;
		for (int i = 0; i < numPrimaryBranches; ++i, ++lineOn)
		{
			wordOn = 0;
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));

			CurrentPrimaryNode = std::atoi(words[wordOn].c_str());
			++wordOn;

			PrimaryBranchSelection = boost::lexical_cast<unsigned int>(words[wordOn]);
			++wordOn;

			PrimaryBranchSelectionValid = true;
			CurrentPredecessors = NodesPredecessors[CurrentPrimaryNode].predecessors;
			SelectPrimaryBranchesOperation();
		}

	}

	void BMetaGraph::saveTraitsToFile(std::string filename)
	{
		std::ofstream filestream;
		filestream.open(filename);

		filestream << "ply" << std::endl;
		filestream << "format ascii 1.0" << std::endl;
		filestream << "element stem " << std::endl;
		filestream << "element primary_nodes " << PrimaryNodes.size() << std::endl;
		filestream << "property int index" << std::endl;
		filestream << "property int32 vertex" << std::endl;
		filestream << "element primary_branches " << PrimaryBranchesObj.size() << std::endl;
		filestream << "property int32 vertex1" << std::endl;
		filestream << "property int32 vertex2" << std::endl;
		filestream << "end_header" << std::endl;
		//mSkeleton.writeDanPly(filestream);

		filestream << selectStemStart << std::endl;
		filestream << selectStemEnd << std::endl;

		for (int it = 0; it < PrimaryNodes.size(); it++)
		{
			filestream << PrimaryNodes[it].first << " " << PrimaryNodes[it].second << std::endl;
		}

		// save primary branches
		for (auto vectorit = PrimaryBranchesObj.begin(); vectorit != PrimaryBranchesObj.end(); ++vectorit)
		{
			filestream << vectorit->primaryNodeIndex << " " << vectorit->branchEndNode << " " << vectorit->metaEdges.size();
			for (MetaE edge : vectorit->metaEdges)
			{
				filestream << " " << edge.m_source;
			}
			filestream << std::endl;
		}

	}

	int BMetaGraph::loadSkeletonFromLines(std::vector<std::string> &lines, int & startingLine)
	{
		return mSkeleton.loadFromLines(lines, startingLine);
	}

	bool BMetaGraph::checkHeaderForMetaInfo(std::vector<std::string> &lines)
	{
		bool end_header_found = false;
		int lineOn = 0;
		std::vector<std::string> words = {};
		while (!end_header_found && lineOn < lines.size())
		{
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
			if (words.size() > 1)
			{
				if (boost::iequals(words[1], "MetaNode"))
				{
					return true;
				}
			}
			else
			{
				if (boost::iequals(words[0], "end_header"))
				{
					end_header_found = true;
					return false;
				}
			}
			++lineOn;
		}
		return false;
	}

	int BMetaGraph::loadGraphFromLines(std::vector<std::string> &lines, int &metaStartingLine)
	{
		int lineOn = 0;
		int numNodes = 0, numEdges = 0, numFaces = 0;
		bool end_header_found = false;
		std::vector<std::string> words = {};
		while (!end_header_found && lineOn < lines.size())
		{
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));

			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "MetaNode"))
			{
				numNodes = boost::lexical_cast<int>(words[2]);
			}
			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "MetaEdge"))
			{
				numEdges = boost::lexical_cast<int>(words[2]);
			}
			if (boost::iequals(words[0], "element") && boost::iequals(words[1], "MetaFace"))
			{
				numFaces = boost::lexical_cast<int>(words[2]);
			}
			if (boost::iequals(words[0], "end_header"))
			{
				end_header_found = true;
			}
			++lineOn;
		}

		lineOn = metaStartingLine;

		words = {};
		boost::split(words, lines[lineOn], boost::is_any_of(" "));
		if (words.size() > 1)
		{
			++lineOn;
		}

		for (int i = 0; i < numNodes; ++i, ++lineOn)
		{
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
			SkelVert srcVert = boost::lexical_cast<unsigned int>(words[0]);
			addNode(srcVert, &mSkeleton);
		}
		int wordOn = 0;
		for (int i = 0; i < numEdges; ++i, ++lineOn)
		{
			wordOn = 0;
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
			MetaV v0 = boost::lexical_cast<unsigned int>(words[wordOn]);
			++wordOn;

			MetaV v1 = boost::lexical_cast<unsigned int>(words[wordOn]);
			++wordOn;
			int numVerts = boost::lexical_cast<int>(words[wordOn]);
			++wordOn;
			std::vector<SkelVert> vertices = {};
			for (int v = 0; v < numVerts; ++v, ++wordOn)
			{
				vertices.push_back(boost::lexical_cast<unsigned int>(words[wordOn]));
			}
			addEdge(v0, v1, vertices, &mSkeleton, true);
		}
		for (int i = 0; i < numFaces; ++i, ++lineOn)
		{
			words = {};
			boost::split(words, lines[lineOn], boost::is_any_of(" "));
		}

		findAndLabelConnectedComponents();
		return lineOn;

	}

	void BMetaGraph::initializeFromSkeleton()
	{
		std::cout << "Building metagraph" << std::endl;
		isLoaded = true;
		std::vector<SkelVert> metaNodes = mSkeleton.getMetaNodes(); // vertices with degree = 1 and degree >= 3
		std::vector<bool> visitedNodes = std::vector<bool>(mSkeleton.m_vertices.size(), false);

		for each(SkelVert node in metaNodes)
		{
			MetaV startV = addNode(node, &mSkeleton);
			BSkeleton::adjacency_iterator adjIt, adjEnd;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(node, mSkeleton);

			for (; adjIt != adjEnd; ++adjIt)
			{
				SkelVert leadVert = *adjIt;
				buildMetaEdge(startV, leadVert, visitedNodes, true);
			}
		}
		std::cout << "Metagraph complete " << std::endl;

		std::cout << "Computing connected components " << std::endl;
		findAndLabelConnectedComponents();
		std::cout << "found connected components " << std::endl;
	}

	MetaE BMetaGraph::buildMetaEdge(MetaV startV, SkelVert &lead, std::vector<bool> &visitedSkelVerts, bool isLoading)
	{
		//if the metaedge leading out of this node towards the lead has already been visited, then
		//we do not need to create a new edge
		if (visitedSkelVerts[lead])
		{
			return MetaE();
		}

		std::vector<SkelVert> edgeVerts = {};
		SkelVert startSkelVert = operator[](startV).mSrcVert;
		edgeVerts.push_back(startSkelVert);
		visitedSkelVerts[startSkelVert] = true;
		SkelVert last = startSkelVert;
		while (boost::degree(lead, mSkeleton) == 2 && lead != startSkelVert)
		{
			edgeVerts.push_back(lead);
			visitedSkelVerts[lead] = true;

			BSkeleton::adjacency_iterator adjIt, adjEnd;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(lead, mSkeleton);
			if (*adjIt != last)
			{
				last = lead;
				lead = *adjIt;
				continue;
			}
			++adjIt;
			if (*adjIt != last)
			{
				last = lead;
				lead = *adjIt;
				continue;
			}

			break;
		}

		edgeVerts.push_back(lead);
		MetaV endV = addNode(lead, &mSkeleton);
		return addEdge(startV, endV, edgeVerts, &mSkeleton, isLoading);
	}

	void BMetaGraph::buildEdgeVBOs()
	{
		std::cout << "buildEdgeVBOs" << std::endl;

		selectionVBO.clear();
		bridgeVBO.clear();
		nonBridgeVBO.clear();
		selectedSegmentVBO.clear();
		primaryBranchesVBO.clear();
		stemVBO.clear();
		TraitsNodes.clear();
		vertexColors.clear();
		vertexColors = { {}, {}, {}, {} };

		whorlEdgeStemVBO.clear();
		nonWhorlEdgeStemVBO.clear();
		selectedWhorlBranchesVBO.clear();
		nonSelectedWhorlBranchesVBO.clear();
		nonStemNonBranchesVBO.clear();

		if (whorls.size() == 0 && auto_stem_metaNode.size() == 0) {
			metaEdgeIter mei = boost::edges(*this);
			for (; mei.first != mei.second; ++mei) {
				BMetaEdge *edge = &operator[](*mei.first);
				edge->updateGraphColors(vertexColors);
				for (int i = 0; i < edge->indicesList.size(); ++i) nonStemNonBranchesVBO.push_back(edge->indicesList[i]);
			}
		}
		else if(whorls.size() == 0) {
			metaEdgeIter mei = boost::edges(*this);
			for (; mei.first != mei.second; ++mei) {
				BMetaEdge *edge = &operator[](*mei.first);
				edge->updateGraphColors(vertexColors);
				MetaE edgeME = *mei.first;
				if (std::find(auto_stem_metaEdge.begin(), auto_stem_metaEdge.end(), edgeME) != auto_stem_metaEdge.end()) {
					for (int i = 0; i < edge->indicesList.size(); ++i) nonWhorlEdgeStemVBO.push_back(edge->indicesList[i]);
				}
				else {
					for (int i = 0; i < edge->indicesList.size(); ++i) nonStemNonBranchesVBO.push_back(edge->indicesList[i]);
				}
			}
		}
		else { // whorls.size() != 0
			std::set<MetaE> allWhorlEdges;
			for (auto it = auto_node.begin(); it != auto_node.end(); ++it) for (MetaE e : std::get<2>(*it)) allWhorlEdges.insert(e);
			metaEdgeIter mei = boost::edges(*this);
			for (; mei.first != mei.second; ++mei) {
				BMetaEdge *edge = &operator[](*mei.first);
				edge->updateGraphColors(vertexColors);
				MetaE edgeME = *mei.first;
				if (allWhorlEdges.find(edgeME) != allWhorlEdges.end()) {
					for (int i = 0; i < edge->indicesList.size(); ++i) whorlEdgeStemVBO.push_back(edge->indicesList[i]);
				}
				else if (std::find(auto_stem_metaEdge.begin(), auto_stem_metaEdge.end(), edgeME) != auto_stem_metaEdge.end()) {
					for (int i = 0; i < edge->indicesList.size(); ++i) nonWhorlEdgeStemVBO.push_back(edge->indicesList[i]);
				}
				else {
					for (int i = 0; i < edge->indicesList.size(); ++i) nonStemNonBranchesVBO.push_back(edge->indicesList[i]);
				}
			}
		}

		//metaEdgeIter mei = boost::edges(*this);
		//for (; mei.first != mei.second; ++mei)
		//{
		//	BMetaEdge *edge = &operator[](*mei.first);
		//	edge->updateGraphColors(vertexColors);
		//	if (onlyDisplaySelectedComponents)
		//	{
		//		if (edge->connectedComponent != selectedComponent1 && edge->connectedComponent != selectedComponent2)
		//		{
		//			continue;
		//		}
		//	}
		//	if (edge->isSelected)
		//	{
		//		for (int i = 0; i < edge->indicesList.size(); ++i)
		//		{
		//			selectionVBO.push_back(edge->indicesList[i]);
		//		}
		//	}
		//	else if (edge->isBridge)
		//	{
		//		for (int i = 0; i < edge->indicesList.size(); ++i)
		//		{
		//			bridgeVBO.push_back(edge->indicesList[i]);
		//			if (edge->indicesList[i] * 3 > mSkeleton.glVertices.size())
		//			{
		//				std::cout << "skeleton vertices exceeded " << std::endl;
		//			}
		//		}
		//	}
		//	else if (!edge->isBridge)
		//	{
		//		for (int i = 0; i < edge->indicesList.size(); ++i)
		//		{
		//			nonBridgeVBO.push_back(edge->indicesList[i]);
		//		}
		//	}
		//}

		//if (stemSelected)
		//{
		//	for (MetaE edge : auto_stem_metaEdge)
		//	{
		//		BMetaEdge *Medge = &operator[](edge);
		//		for (int i = 0; i < Medge->indicesList.size(); ++i)
		//		{
		//			stemVBO.push_back(Medge->indicesList[i]);
		//		}
		//	}
		//}

		//// push back to selectedSegmentVBO
		//if (SegmentMetaEdges.size() > 0)
		//{
		//	for (MetaE edge : SegmentMetaEdges)
		//	{
		//		BMetaEdge *Medge = &operator[](edge);
		//		for (int i = 0; i < Medge->indicesList.size(); ++i)
		//		{
		//			selectedSegmentVBO.push_back(Medge->indicesList[i]);
		//		}
		//	}
		//}

		//// push back to primaryBranchesVBO
		//if (PrimaryBranchesObj.size() > 0)
		//{
		//	for (auto vectorit = PrimaryBranchesObj.begin(); vectorit != PrimaryBranchesObj.end(); ++vectorit)
		//	{
		//		for (MetaE edge : vectorit->metaEdges)
		//		{
		//			BMetaEdge *Medge = &operator[](edge);
		//			for (int i = 0; i < Medge->indicesList.size(); ++i)
		//			{
		//				primaryBranchesVBO.push_back(Medge->indicesList[i]);
		//			}
		//			MetaV v0 = edge.m_source;
		//			if (std::find(TraitsNodes.begin(), TraitsNodes.end(), v0) == TraitsNodes.end())
		//			{
		//				TraitsNodes.push_back(v0);
		//			}
		//			MetaV v1 = edge.m_target;
		//			if (std::find(TraitsNodes.begin(), TraitsNodes.end(), v1) == TraitsNodes.end())
		//			{
		//				TraitsNodes.push_back(v1);
		//			}
		//		}
		//	}
		//}
	}

	MetaV BMetaGraph::addNode(SkelVert srcVert, BSkeleton *srcSkel)
	{
		if (vertNodeMap.count(srcVert) == 0)
		{
			BMetaNode node = BMetaNode(srcVert, srcSkel);
			MetaV v = boost::add_vertex(*this);

			operator[](v) = node;
			vertNodeMap[srcVert] = v;
			nodeVertMap[v] = srcVert;
		}
		return vertNodeMap[srcVert];
	}

	MetaE BMetaGraph::addEdge(MetaV v0, MetaV v1, std::vector<SkelVert> skelVerts, BSkeleton *srcSkel, bool isLoading, bool isJoining)
	{
		BMetaEdge edge;
		if (!isJoining)
		{
			edge = BMetaEdge(skelVerts, srcSkel);
		}
		else
		{
			edge = BMetaEdge(skelVerts, srcSkel);
		}
		MetaE e;
		bool success;
		boost::tie(e, success) = boost::add_edge(v0, v1, *this); // *this is graph

		if (success)
		{
			operator[](e) = edge;
		}

		if (!isLoading)
		{
			std::cout << "finding bridges " << std::endl;
			findBridges();
		}
		return e;
	}

	int BMetaGraph::getNumGLEdges()
	{
		return 100;
	}
	int BMetaGraph::getNumGLVertices()
	{
		return 100;
	}

	void BMetaGraph::duplicateEdge(MetaE e0, BSkeleton *srcSkel, MetaV &dupV0, MetaV &dupV1, MetaE &dupE)
	{
		BMetaEdge edge = operator[](e0);

		std::vector<SkelVert> addedVerts = {};

		//iterate over source vertices, and add copies of their information to the skeleton
		if (edge.mVertices.size() > 0)
		{
			for each(SkelVert v in edge.mVertices)
			{
				addedVerts.push_back(srcSkel->addVertex(srcSkel->getVertData(v)));
			}

			bool exists = false;
			std::vector<SkelEdge> srcEdges = {};
			//find all the edge data associated with the original set of vertices
			for (int i = 0; i < edge.mVertices.size() - 1; ++i)
			{
				SkelEdge e = srcSkel->getEdge(edge.mVertices[i], edge.mVertices[i + 1], exists);
				if (exists)
				{
					srcEdges.push_back(e);
				}
			}

			//add edges between all of the new added vertices containing the info from the original edge data
			for (int i = 0; i < edge.mVertices.size() - 1; ++i)
			{
				srcSkel->addEdge(addedVerts[i], addedVerts[i + 1]);
			}
		}

		//add metanodes and edges for the augmented elements in the skeleton
		dupV0 = addNode(addedVerts[0], srcSkel);
		dupV1 = addNode(addedVerts[addedVerts.size() - 1], srcSkel);
		MetaV srcNode0 = vertNodeMap[edge.mVertices[0]];
		MetaV srcNode1 = vertNodeMap[edge.mVertices[edge.mVertices.size() - 1]];
		dupE = addEdge(dupV0, dupV1, addedVerts, srcSkel, false);


		mDuplicateEdgeMap[dupE] = e0;
		mDuplicateNodeMap[dupV0] = srcNode0;
		mDuplicateNodeMap[dupV1] = srcNode1;
		return;
	}

	void BMetaGraph::removeNode(MetaV nodeToRemove)
	{
		BMetaNode toRemove = operator[](nodeToRemove);
		vertNodeMap.erase(toRemove.mSrcVert);
		boost::remove_vertex(nodeToRemove, *this);
		updateVertNodeMap();
	}

	void BMetaGraph::removeEdge(MetaE edgeToRemove)
	{
		std::cout << "unselecting" << std::endl;
		unselectEdge(edgeToRemove);
		std::cout << "boost removal " << std::endl;
		boost::remove_edge(edgeToRemove, *this);
		std::cout << "Edge removal successfull" << std::endl;
	}

	void BMetaGraph::removeEdgeNoBridging(MetaE edgeToRemove)
	{
		MetaV v0 = std::min(edgeToRemove.m_source, edgeToRemove.m_target);
		MetaV v1 = std::max(edgeToRemove.m_source, edgeToRemove.m_target);

		boost::remove_edge(edgeToRemove, *this);

		if (boost::degree(v1, *this) == 0)
		{
			removeNode(v1);
		}
		if (boost::degree(v0, *this) == 0)
		{
			removeNode(v0);
		}
	}

	void BMetaGraph::updateVertNodeMap()
	{
		vertNodeMap.clear();

		metaVertIter mvi = boost::vertices(*this);
		for (; mvi.first != mvi.second; ++mvi)
		{
			MetaV  node = *mvi.first;
			SkelVert vert = operator[](node).mSrcVert;
			vertNodeMap[vert] = node;
		}
	}

	void BMetaGraph::updateNodeDegrees()
	{
		return;
	}

	void BMetaGraph::findAndLabelConnectedComponents()
	{
		std::cout << "===== Finding Connected Components =====" << std::endl;
		//build the boost component map vector<int> mapping vertices to components
		mComponentMap.resize(m_vertices.size(), 0);
		std::cout << "m_vertices.size() " << m_vertices.size() << std::endl;
		size_t numVertices = boost::num_vertices(*this);

		numComponents = boost::connected_components(*this, &mComponentMap[0]);
		for (int i = 0; i < mComponentMap.size(); ++i)
		{
			numComponents = std::max(numComponents, mComponentMap[i] + 1);
		}
		std::cout << "number of components : " << numComponents << std::endl;
		mComponentSizeMap = {};
		mComponentSizeMap.resize(numComponents, 0);

		//build the component size map by determining for each edge the assigned component 
		//of its nodes, and adding its length to the cumulative size for each component
		metaEdgeIter mei = boost::edges(*this);
		for (; mei.first != mei.second; ++mei)
		{
			MetaV node0 = mei.first->m_source;
			//std::cout << "node : first - " << mei.first->m_source << " second - " << std::endl;
			int component = mComponentMap[node0];
			mComponentSizeMap[component] += operator[](*mei.first).mLength;
			//std::cout << "mComponentSizeMap[" << component << "] = " << mComponentSizeMap[component] << std::endl;
		}
		std::cout << "component size map built" << std::endl;

		//sort the components by their size (cumulative edge length)
		std::vector<float> allSizes = {};
		allSizes.insert(allSizes.begin(), mComponentSizeMap.begin(), mComponentSizeMap.end());


		std::sort(allSizes.begin(), allSizes.end());

		//map from current component Ids to prioritized component Ids - the highest length components
		//will have the highest priorty (iterate backwards on allsizes)
		std::map<int, int> componentPriorityMap = std::map<int, int>();
		int priority = 0;
		for (int i = allSizes.size() - 1; i >= 0; --i)
		{
			std::cout << "i = " << i << std::endl;
			for (int component = 0; component < mComponentSizeMap.size(); ++component)
			{
				if (mComponentSizeMap[component] == allSizes[i])
				{
					componentPriorityMap[component] = priority;
					std::cout << "component priority map : component " << component << " has priority " << priority << std::endl;
					++priority;
				}
				if (priority == numComponents)
				{
					break;
				}
			}
		}

		//remap the existing component map to the prioritized component map
		for (int i = 0; i < m_vertices.size(); ++i)
		{
			mComponentMap[i] = componentPriorityMap[mComponentMap[i]];
		}
		//update the component size map to match the sorted sizes of the priority map
		std::cout << "Component sizes " << std::endl;
		for (int i = 0, j = allSizes.size() - 1; i < allSizes.size(), j >= 0; ++i, --j)
		{
			mComponentSizeMap[i] = allSizes[j];
			std::cout << i << " " << mComponentSizeMap[i] << std::endl;
		}


		//remap the components of the nodes and edges locally
		std::cout << "remap the components of the nodes and edges locally " << std::endl;
		for (MetaV node = 0; node < m_vertices.size(); ++node)
		{
			operator[](node).connectedComponent = mComponentMap[node];
			operator[](node).updateComponentColor();
		}
		std::cout << "component color updated " << std::endl;

		mei = boost::edges(*this);
		for (; mei.first != mei.second; ++mei)
		{
			MetaV node0 = mei.first->m_source;
			int component = mComponentMap[node0];
			operator[](*mei.first).connectedComponent = component;
			operator[](*mei.first).updateComponentColor(vertexColors);
		}
		MinMaxStruct::minComponent = 0;
		MinMaxStruct::maxComponent = numComponents - 1;
		std::cout << "updated node component" << std::endl;

		//componentBounds = std::vector<BoundingBox>(numComponents, BoundingBox());

		//metaVertIter mvi = boost::vertices(*this);
		//for (; mvi.first != mvi.second; ++mvi)
		//{
		//	BMetaNode *node = &operator[](*mvi.first);
		//	componentBounds[node->connectedComponent].addPoint(node->p);
		//}
		//std::cout << "===== Components found =====" << std::endl;
	}

	boost::python::list BMetaGraph::getComponentSizes()
	{
		return toPythonList<float>(mComponentSizeMap);
	}

	bool BMetaGraph::isDisplaySelectedSegment()
	{
		return showSelectedSegment;
	}

	void BMetaGraph::ConnectComponents(int compOne, int compTwo)
	{
		int lowerComponent = std::min(compOne, compTwo);
		int higherComponent = std::max(compOne, compTwo);
		metaVertIter mvi = boost::vertices(*this);

		for (; mvi.first != mvi.second; ++mvi)
		{
			int nodeComponent = operator[](*mvi.first).connectedComponent;
			if (nodeComponent == compOne || nodeComponent == compTwo)
			{
				operator[](*mvi.first).connectedComponent = lowerComponent;
			}
			if (nodeComponent > higherComponent)
			{
				operator[](*mvi.first).connectedComponent--;
			}
		}
	}

	void BMetaGraph::FixUpComponents()
	{
		metaVertIter mvi = boost::vertices(*this);

		for (; mvi.first != mvi.second; ++mvi)
		{
			int nodeComponent = operator[](*mvi.first).connectedComponent;
			BMetaGraph::adjacency_iterator adjIt, adjEnd;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(*mvi.first, *this);
			for (; adjIt != adjEnd; ++adjIt)
			{
				int neighborComponent = operator[](*adjIt).connectedComponent;
				if (neighborComponent != nodeComponent)
				{
					ConnectComponents(nodeComponent, neighborComponent);
				}
			}
		}
	}

	void BMetaGraph::FindMinimumSpanningTree()
	{
		mMinimumSpanningTree = {};

		kruskal_minimum_spanning_tree(*this, std::back_inserter(mMinimumSpanningTree),
			weight_map(get(&BMetaEdge::averageThickness, *this)));
	}

	int BMetaGraph::GetNumEdgesToBreak()
	{
		FindMinimumSpanningTree();
		return m_edges.size() - mMinimumSpanningTree.size();
	}

	void BMetaGraph::JoinOperation()
	{

		if (!selectNode1Valid || !selectNode2Valid)
		{
			return;
		}

		Point3d p1picked = mSkeleton[operator[](selectNode1).mSrcVert];
		Point3d p2picked = mSkeleton[operator[](selectNode2).mSrcVert];


		RootAttributes newAttributes = RootAttributes(p1picked.id, p2picked.id, p1picked, p2picked);

		SkelEdge newSkelEdge = mSkeleton.addEdge(p1picked.id, p2picked.id);

		//create an edge between these nodes and add it to the metagraph
		std::vector<SkelVert> bridgeVerts = {};
		bridgeVerts.push_back(operator[](selectNode1).mSrcVert);
		bridgeVerts.push_back(operator[](selectNode2).mSrcVert);
		MetaV node1 = vertNodeMap[p1picked.id];
		MetaV node2 = vertNodeMap[p2picked.id];

		if (boost::degree(node1, *this) == 0)
		{
			PromoteOperation(p1picked.id);
		}
		if (boost::degree(node2, *this) == 0)
		{
			PromoteOperation(p2picked.id);
		}


		MetaE newMetaEdge = addEdge(node1, node2, bridgeVerts, &mSkeleton, false, false);

		unselectAll();

		findAndLabelConnectedComponents();
		operator[](newMetaEdge).updateColors(edgeOptions, vertexColors, &mSkeleton);

		if (node1 < node2)
		{
			BridgeNode(node2);
			BridgeNode(node1);
		}
		else
		{
			BridgeNode(node1);
			BridgeNode(node2);
		}

		findAndLabelConnectedComponents();
		buildEdgeVBOs();
		std::cout << "Finished joining " << std::endl;
		return;
	}

	void BMetaGraph::BreakOperation()
	{
		std::cout << "C++ break operation " << std::endl;
		if (!breakEdgeValid)
		{
			return;
		}

		MetaV v0 = std::min(breakEdge.m_source, breakEdge.m_target);
		MetaV v1 = std::max(breakEdge.m_source, breakEdge.m_target);

		MetaE breakEdgeE;
		bool exists = false;

		boost::tie(breakEdgeE, exists) = boost::edge(v0, v1, *this);

		if (!exists)
		{
			std::cout << "Break edge doesnt exist " << std::endl;
			return;
		}
		unselectEdge(breakEdge);
		removeEdge(breakEdgeE);
		breakEdgeValid = false;


		if (boost::degree(v1, *this) == 0)
		{
			removeNode(v1);
		}
		else
		{
			BridgeNode(v1);
		}
		if (boost::degree(v0, *this) == 0)
		{
			removeNode(v0);
		}
		else
		{
			BridgeNode(v0);
		}
		findBridges();
		findAndLabelConnectedComponents();
		buildEdgeVBOs();
		std::cout << "Break operation completed" << std::endl;
		return;
	}

	void BMetaGraph::SplitOperation()
	{
		if (!splitEdgeValid)
		{
			return;
		}
		bool sourceCovered = false, targetCovered = false;
		for (MetaE edge : splitNeighbors)
		{
			if (edge.m_source == splitEdge.m_target || edge.m_target == splitEdge.m_target)
			{
				targetCovered = true;
			}
			if (edge.m_source == splitEdge.m_source || edge.m_target == splitEdge.m_target)
			{
				sourceCovered = true;
			}
		}
		if (!sourceCovered || !targetCovered)
		{
			return;
		}
		unselectEdge(splitEdge);
		for (MetaE edge : splitNeighbors)
		{
			unselectEdge(edge);
		}


		MetaV node0 = splitEdge.m_source;
		MetaV node1 = splitEdge.m_target;
		MetaE srcE;

		bool exists = false;

		boost::tie(srcE, exists) = boost::edge(splitEdge.m_source, splitEdge.m_target, *this);

		if (!exists)
		{
			return;
		}

		BMetaEdge toSplit = operator[](srcE);

		float shiftX = 0, shiftY = 0, shiftZ = 0;

		for (MetaE originalEdge : splitNeighbors)
		{

			MetaV connectionToSplit;
			//if the srcEdge node0 is equal to either of the nodes on the splitting edge, then 
			//we will break the connection between srcEdge node0 and node1, and create a new edge
			//between node0 and the new target node (determined by mapping of the duplicated nodes
			//to the old target node of the srcEdge
			if (originalEdge.m_source == splitEdge.m_source || originalEdge.m_source == splitEdge.m_target)
			{
				connectionToSplit = originalEdge.m_source;
			}
			else
			{
				connectionToSplit = originalEdge.m_target;
			}

			MetaE neighbor;
			boost::tie(neighbor, exists) = boost::edge(originalEdge.m_source, originalEdge.m_target, *this);
			BMetaEdge nedge = operator[](neighbor);
			SkelVert shiftTowards;
			if (vertNodeMap[nedge.start()] == connectionToSplit)
			{
				shiftTowards = nedge.mVertices[1];
			}
			else
			{
				shiftTowards = nedge.mVertices[nedge.mVertices.size() - 2];
			}

			float* goTowards = mSkeleton[shiftTowards].p;
			float* from = operator[](connectionToSplit).p;
			std::cout << "from - x : " << from[0] << " y : " << from[1] << " z : " << from[2] << std::endl;
			std::cout << "to - x : " << goTowards[0] << " y : " << goTowards[1] << " z : " << goTowards[2] << std::endl;
			float deltaX = goTowards[0] - from[0];
			float deltaY = goTowards[1] - from[1];
			float deltaZ = goTowards[2] - from[2];
			shiftX += deltaX * 0.75;
			shiftY += deltaY * 0.75;
			shiftZ += deltaZ * 0.75;
		}

		Point3d shift = Point3d(shiftX, shiftY, shiftZ, 0, 0);
		std::vector<SkelVert> splitVerts = toSplit.mVertices;
		std::vector<RootAttributes> splitEdges = toSplit.mEdges;
		std::map<MetaV, MetaV> originalToDuplicateMap = std::map<MetaV, MetaV>();

		std::vector<SkelVert> newVertices = {};

		for (int i = 0; i < splitVerts.size(); ++i)
		{
			Point3d src = mSkeleton[splitVerts[i]];
			Point3d dst = src + shift;
			newVertices.push_back(mSkeleton.addVertex(dst));
		}

		for (int i = 0; i < newVertices.size() - 1; ++i)
		{
			SkelEdge added = mSkeleton.addEdge(newVertices[i], newVertices[i + 1]);
		}

		MetaV v0 = addNode(newVertices[0], &mSkeleton);
		MetaV v1 = addNode(newVertices.back(), &mSkeleton);

		originalToDuplicateMap[vertNodeMap[splitVerts[0]]] = v0;
		originalToDuplicateMap[vertNodeMap[splitVerts.back()]] = v1;

		MetaE addedEdge = addEdge(v0, v1, newVertices, &mSkeleton, false);

		for (MetaE originalEdge : splitNeighbors)
		{

			MetaV splitNode, remainingNode;
			//if the srcEdge node0 is equal to either of the nodes on the splitting edge, then 
			//we will break the connection between srcEdge node0 and node1, and create a new edge
			//between node0 and the new target node (determined by mapping of the duplicated nodes
			//to the old target node of the srcEdge


			MetaE e;
			bool exists = false;
			boost::tie(e, exists) = boost::edge(originalEdge.m_source, originalEdge.m_target, *this);

			std::vector<SkelVert> originalVerts = {};
			for (SkelVert v : operator[](e).mVertices)
			{
				originalVerts.push_back(v);
			}
			std::vector<RootAttributes> originalEdges = operator[](e).mEdges;

			if (originalEdge.m_source == splitEdge.m_source || originalEdge.m_source == splitEdge.m_target)
			{
				splitNode = originalEdge.m_source;
				remainingNode = originalEdge.m_target;
			}
			else
			{
				splitNode = originalEdge.m_target;
				remainingNode = originalEdge.m_source;
			}

			if (vertNodeMap[originalVerts[0]] == splitNode)
			{
				originalVerts[0] = operator[](originalToDuplicateMap[splitNode]).mSrcVert;
				mSkeleton.addEdge(originalVerts[0], originalVerts[1]);
			}
			else
			{
				originalVerts[originalVerts.size() - 1] = operator[](originalToDuplicateMap[splitNode]).mSrcVert;
				mSkeleton.addEdge(originalVerts[originalVerts.size() - 1], originalVerts[originalVerts.size() - 2]);
			}


			removeEdge(e);
			addEdge(remainingNode, originalToDuplicateMap[splitNode], originalVerts, &mSkeleton, false, false);
		}


		//attempt to bridge the nodes at either end of both sides of the split
		//eg. if both the duplicated edge and the src edge will now have a single edge
		//connected at 1 end, then join those two edges together

		std::vector<MetaV> orderedNodes = { node0, node1, v0, v1 };
		std::sort(orderedNodes.begin(), orderedNodes.end());

		findAndLabelConnectedComponents();

		for (int i = orderedNodes.size() - 1; i >= 0; --i)
		{
			BridgeNode(orderedNodes[i]);
		}
		mSkeleton.updateGLVertices();
		buildEdgeVBOs();
		return;
	}

	void BMetaGraph::RemoveComponentOperation()
	{
		std::cout << "Remove Component Mode " << std::endl;
		if (!removeComponentEdgeValid)
		{
			return;
		}

		MetaV removeComponentNode = removeComponentEdge.m_source;
		int removeComponent = mComponentMap[removeComponentNode];
		std::cout << "remove component ID " << removeComponent << std::endl;
		int nodeComponent;

		unselectEdge(removeComponentEdge);
		removeComponentEdgeValid = false;

		metaEdgeIter mei = boost::edges(*this);
		std::list<MetaE> removeEdgeList;
		MetaE removeComponentEdgeE;
		bool exists = false;
		for (; mei.first != mei.second; ++mei)
		{
			if (operator[](*mei.first).connectedComponent == removeComponent)
			{
				MetaV node0 = std::min(mei.first->m_source, mei.first->m_target);
				MetaV node1 = std::max(mei.first->m_source, mei.first->m_target);
				boost::tie(removeComponentEdgeE, exists) = boost::edge(node0, node1, *this);

				if (!exists)
				{
					std::cout << "Remove component edge doesnt exist " << std::endl;
					continue;
				}
				std::cout << "remove edge " << removeComponentEdgeE << std::endl; // (98,458)
				removeEdgeList.push_back(removeComponentEdgeE);
			}
		}
		while (!removeEdgeList.empty())
		{
			removeComponentEdgeE = removeEdgeList.back();
			removeEdgeList.pop_back();
			removeEdge(removeComponentEdgeE); //error?
		}

		int test = 0;
		int count = 0;
		std::cout << "m_vertices.size() " << m_vertices.size() << std::endl;
		for (MetaV node = 0; node < m_vertices.size(); ++node)
		{
			nodeComponent = operator[](node).connectedComponent;

			if (nodeComponent == removeComponent)
			{
				std::cout << "remove " << node << std::endl;
				removeNode(node);
				node--;
				count++;
			}
		}
		std::cout << count << " vertices will be removed " << std::endl;
		std::cout << "m_vertices.size() " << m_vertices.size() << std::endl;

		findAndLabelConnectedComponents();
		buildEdgeVBOs();
		std::cout << "Remove component operation completed" << std::endl;
		return;

	}

	float BMetaGraph::getEdgeEuclidLength(SkelEdge srcId, BSkeleton *skel)
	{
		return skel->operator[](srcId).euclidLength;
	}

	float BMetaGraph::getVertThickness(SkelVert srcId, BSkeleton *skel)
	{
		float Thickness = skel->operator[](srcId).thickness();
		return Thickness;
	}

	float BMetaGraph::getVertWidth(SkelVert srcId, BSkeleton *skel)
	{
		float Width = skel->operator[](srcId).width();
		return Width;
	}

	float BMetaGraph::getRelativeDistance(SkelVert sv1, SkelVert sv2) {
		float deltaX = (&mSkeleton)->operator[](sv1).x() - (&mSkeleton)->operator[](sv2).x();
		float deltaY = (&mSkeleton)->operator[](sv1).y() - (&mSkeleton)->operator[](sv2).y();
		float deltaZ = (&mSkeleton)->operator[](sv1).z() - (&mSkeleton)->operator[](sv2).z();
		return std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
	}

	bool BMetaGraph::vertexIsOutOfRadiusRange(SkelVert src, std::deque<SkelVert> stem, float timesThickness) {
		float buffer = FLT_MAX;
		SkelVert svBuffer = -1;
		for (SkelVert sv : stem) {
			float dist = getRelativeDistance(src, sv);
			if (dist < buffer) {
				buffer = dist;
				svBuffer = sv;
			}
		}
		float thickness = getVertThickness(svBuffer, &mSkeleton);
		return buffer > thickness * timesThickness;
	}

	void BMetaGraph::SelectStemOperation()
	{
		if (!selectStemStartValid || !selectStemEndValid)
		{
			return;
		}
		std::cout << "Select Stem Operation " << std::endl;

		// clear currently stored stem and visualization
		autoStemVBO.clear();
		auto_stem.clear();
		auto_stem_metaNode.clear();
		auto_stem_metaEdge.clear();
		whorls.clear();
		auto_node.clear();
		showWhorl = false;
		mode = OperationMode::None;

		//StemPath.clear();
		StemPath_node.clear();

		using weight_map_t = boost::property_map<BMetaGraph, float BMetaEdge::*>::type;
		weight_map_t kWeightMap = boost::get(&BMetaEdge::mLength, *this);

		std::vector<int> distances(boost::num_vertices(*this));
		std::vector<MetaV> predecessors(boost::num_vertices(*this));

		boost::dijkstra_shortest_paths(*this, selectStemStart,
			boost::predecessor_map(boost::make_iterator_property_map(predecessors.begin(), boost::get(boost::vertex_index, *this)))
			.distance_map(boost::make_iterator_property_map(distances.begin(), boost::get(boost::vertex_index, *this)))
			.weight_map(kWeightMap)
		);

		// extract the shortest path from start to end.
		MetaV v = selectStemEnd;
		for (MetaV u = predecessors[v]; u != v; v = u, u = predecessors[v])
		{
			auto_stem_metaEdge.push_back(boost::edge(u, v, *this).first);
		}
		MetaV v_tmp;
		for (std::vector<MetaE>::reverse_iterator riter = auto_stem_metaEdge.rbegin(); riter != auto_stem_metaEdge.rend(); ++riter)
		{
			MetaV u_tmp = boost::source(*riter, *this);
			v_tmp = boost::target(*riter, *this);
			MetaE e_tmp = boost::edge(u_tmp, v_tmp, *this).first;
			auto_stem_metaNode.push_back(u_tmp);
		}
		auto_stem_metaNode.push_back(v_tmp);
		stemSelected = true;
		selectSegmentPoint1 = selectStemStart;
		selectSegmentPoint2 = selectStemEnd;
		SkelVert prev = -1;
		MetaV mPrev = -1;
		for (MetaV mCurr : auto_stem_metaNode) {
			if (mPrev != -1) {
				MetaE e = boost::edge(mPrev, mCurr, *this).first;
				std::vector<SkelVert> subVerts = operator[](e).mVertices;
				if (subVerts[0] != operator[](mPrev).mSrcVert) {
					std::reverse(subVerts.begin(), subVerts.end());
				}
				for (SkelVert curr : subVerts) {
					if (prev != -1) {
						autoStemVBO.push_back(prev);
						autoStemVBO.push_back(curr);
					}
					prev = curr;
				}
			}
			mPrev = mCurr;
		}
		unselectAll();
	}

	void BMetaGraph::FindStemOperation(float lowThreshold)
	{
		if (!isLoaded) return;

		std::cout << "Automatically detecting stem. Low threshold: " << lowThreshold << std::endl;

		// clear currently stored stem and visualization
		autoStemVBO.clear();
		auto_stem.clear();
		auto_stem_metaNode.clear();
		auto_stem_metaEdge.clear();
		whorls.clear();
		auto_node.clear();
		showWhorl = false;
		mode = OperationMode::None;

		// find out vertices with highest width
		float maxWidth = 0.0;
		SkelVert vertMaxWidth;
		for (SkelVertIterator svi = boost::vertices(mSkeleton); svi.first != svi.second; ++svi)
		{
			SkelVert vert = *svi.first;
			if (getVertWidth(vert, &mSkeleton) > maxWidth)
			{
				vertMaxWidth = vert;
				maxWidth = getVertWidth(vert, &mSkeleton);
			}
		}

		// start from the vertex with highest width, perform BFS
		// to find all connected vertices whose thickness >= lowThreshold
		std::set<SkelVert> visited;
		std::deque<SkelVert> queVert;

		std::deque<SkelVert> selectedVert;
		std::deque<SkelEdge> selectedEdge;

		visited.insert(vertMaxWidth);
		queVert.push_back(vertMaxWidth);
		selectedVert.push_back(vertMaxWidth);

		while (!queVert.empty())
		{
			SkelVert curr = queVert.front();
			queVert.pop_front();

			BSkeleton::adjacency_iterator adjIt, adjEnd;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(curr, mSkeleton);
			for (; adjIt != adjEnd; ++adjIt)
			{
				SkelVert next = *adjIt;
				if (getVertWidth(next, &mSkeleton) >= lowThreshold && visited.count(next) == 0)
				{
					visited.insert(next);
					queVert.push_back(next);
					selectedVert.push_back(next);
					selectedEdge.push_back(boost::edge(curr, next, mSkeleton).first);
				}
			}
		}

		// sort edges by length
		std::sort(selectedEdge.begin(), selectedEdge.end(),
			[&](const SkelEdge& e1, const SkelEdge& e2) {
			return getEdgeEuclidLength(e1, &mSkeleton) < getEdgeEuclidLength(e2, &mSkeleton);
		});
		// minimum spainning tree - Kruskal
		DisjointSets ds(boost::num_vertices(mSkeleton));
		std::vector<SkelEdge> stemMinimumSpanningTree;

		for (int i = 0; i < selectedEdge.size(); ++i)
		{
			int u = selectedEdge[i].m_source;
			int v = selectedEdge[i].m_target;

			int set_u = ds.find(u);
			int set_v = ds.find(v);

			// check if the selected edge is creating a cycle or not 
			// (Cycle is created if u and v belong to same set) 
			if (set_u != set_v)
			{
				// current edge will be in the MST, so print it 
				ds.merge(set_u, set_v);		// merge two sets 
				stemMinimumSpanningTree.push_back(selectedEdge[i]);
			}
		}
		std::cout << "Minimum spanning tree size: " << stemMinimumSpanningTree.size() << std::endl;
		if (stemMinimumSpanningTree.size() == 0) return;

		// burn time to a single point. inverse burn: start with highest burn time.
		// only look at node with one minus current burn time
		typedef property<edge_weight_t, float> EdgeWeightProperty;
		typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty> subGraph;
		subGraph mstGraph;
		typedef boost::graph_traits<subGraph>::vertex_descriptor subV;
		typedef boost::graph_traits<subGraph>::edge_descriptor subE;
		typedef boost::graph_traits<subGraph>::vertex_iterator subVertIter;
		typedef boost::graph_traits<subGraph>::edge_iterator subEdgeIter;
		// https://stackoverflow.com/questions/24366642/how-do-i-change-the-edge-weight-in-a-graph-using-the-boost-graph-library
		typedef subGraph::edge_descriptor Edge;
		// map mst subGraph to skeleton vertex
		std::vector<int> SkelVertMSTMap(mSkeleton.m_vertices.size(), -1);

		std::vector<int> MSTSkelVertMap(selectedVert.size(), -1);
		for (int i = 0; i < selectedVert.size(); ++i) // add vertices to mst graph
		{
			int vert = boost::add_vertex(mstGraph);
			int skelV = selectedVert[i];
			SkelVertMSTMap[skelV] = vert;
			MSTSkelVertMap[vert] = skelV;
		}
		for (int i = 0; i < stemMinimumSpanningTree.size(); ++i) // add edges to mst graph
		{
			SkelEdge e;
			bool success;
			int v0 = stemMinimumSpanningTree[i].m_source;
			int v1 = stemMinimumSpanningTree[i].m_target;
			boost::tie(e, success) = boost::add_edge(SkelVertMSTMap[v0], SkelVertMSTMap[v1], mstGraph);
			std::pair<Edge, bool> edge = boost::edge(SkelVertMSTMap[v0], SkelVertMSTMap[v1], mstGraph);
			float weight = ((&mSkeleton)->operator[](v0).width() + (&mSkeleton)->operator[](v1).width()) / 2;
			boost::put(boost::edge_weight_t(), mstGraph, edge.first, weight);
		}

		// burning
		subGraph burningGraph = mstGraph;
		std::vector<std::vector<int>> burnTimeVertMap; // burn time to all vertices
		std::vector<std::vector<subE>> burnTimeEdgeMap; // burn time to all edges
		std::map<subE, int> edgeToBurnround; // edge mapped to burnrounds
		std::vector<int> skelVertBTmap(boost::num_vertices(burningGraph), -1); // mst graph vert to burn time
		int burnRound = 0;
		bool burnable = true;
		int count = 0;
		//burn edges
		while (burnable)
		{
			burnTimeVertMap.push_back(std::vector<int>());
			burnTimeEdgeMap.push_back(std::vector<subE>());
			burnable = false;
			std::vector<subE> subGraphEdges;
			subEdgeIter ei, ei_end;
			std::vector<subE> burnedEdges;
			float minThickness = 100; // find the minimum thickness for the end edges of this round
			for (std::tie(ei, ei_end) = boost::edges(burningGraph); ei != ei_end; ++ei) {
				bool exists;
				subE e;
				boost::tie(e, exists) = boost::edge(ei->m_source, ei->m_target, burningGraph);
				float weightE = get(boost::edge_weight_t(), burningGraph, e);
				if (!(boost::degree(ei->m_source, burningGraph) > 1 && boost::degree(ei->m_source, burningGraph) > 1) && weightE < minThickness) {
					minThickness = weightE;
				}
			}
			for (std::tie(ei, ei_end) = boost::edges(burningGraph); ei != ei_end; ++ei)
			{
				bool exists;
				subE e;
				boost::tie(e, exists) = boost::edge(ei->m_source, ei->m_target, burningGraph);

				if (!exists)
					continue;
				// this edge exists in the burning Graph
				// this is an inner edge
				std::pair<Edge, bool> ed = boost::edge(ei->m_source, ei->m_target, burningGraph);
				float weight = get(boost::edge_weight_t(), burningGraph, ed.first);
				if (boost::degree(ei->m_source, burningGraph) > 1
					&& boost::degree(ei->m_target, burningGraph) > 1)
				{
					subGraphEdges.push_back(e);
				}
				else if (weight > minThickness) {
					boost::put(boost::edge_weight_t(), burningGraph, ed.first, weight - minThickness);
					subGraphEdges.push_back(e);
					burnable = true;
				}
				else
				{

					burnTimeEdgeMap[burnRound].push_back(boost::edge(ei->m_source, ei->m_target, mstGraph).first);
					edgeToBurnround[boost::edge(ei->m_source, ei->m_target, mstGraph).first] = burnRound;
					if (boost::degree(ei->m_source, burningGraph) == 1)
					{
						skelVertBTmap[ei->m_source] = burnRound;
						burnTimeVertMap[burnRound].push_back(ei->m_source);
					}
					if (boost::degree(ei->m_target, burningGraph) == 1)
					{
						skelVertBTmap[ei->m_target] = burnRound;
						burnTimeVertMap[burnRound].push_back(ei->m_target);
					}

					burnedEdges.push_back(e);
					burnable = true;
				}

			}
			count += burnedEdges.size();
			++burnRound;

			if (subGraphEdges.size() < 1)
			{
				burnable = false;
			}

			// if rest edges in burningGraph intersect at the same vertex
			for (subE e : burnedEdges)
			{
				boost::remove_edge(e, burningGraph);
			}
		}
		std::cout << "total burned edges are " << count << std::endl;
		// inverse burning 
		--burnRound;
		std::vector<subE> BurnEdges = burnTimeEdgeMap[burnRound];
		std::vector<subE> inverseBurnEdges;
		std::vector<int> seedVertices = burnTimeVertMap[burnRound];
		int numEdge = 0;
		if (BurnEdges.size() == 1) { // only one edge at the end

			inverseBurnEdges.push_back(BurnEdges[0]);
			std::vector<int> nextVertices = seedVertices;
			burnRound--;
			while (burnRound > 0) {
				std::vector<int> nextVertices;

				for (int i = 0; i < seedVertices.size(); i++) {
					int bestSeed = -1;
					subE bestEdge;
					subGraph::adjacency_iterator adjIt, adjEnd;
					boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(seedVertices[i], mstGraph);
					int largestRound = -1;
					for (; adjIt != adjEnd; ++adjIt) {
						int v = *adjIt;
						bool exists;
						subE e;
						boost::tie(e, exists) = boost::edge(seedVertices[i], v, mstGraph);
						if (!exists || std::find(inverseBurnEdges.begin(), inverseBurnEdges.end(), e) != inverseBurnEdges.end())
						{
							continue;
						}
						if (edgeToBurnround[e] > largestRound) {
							if (e.m_source != seedVertices[i]) {
								bestSeed = e.m_source;
								bestEdge = e;
								largestRound = edgeToBurnround[e];
							}
							else {
								bestSeed = e.m_target;
								bestEdge = e;
								largestRound = edgeToBurnround[e];
							}
						}
					}

					if (bestSeed > -1) {
						nextVertices.push_back(bestSeed);
						inverseBurnEdges.push_back(bestEdge);
					}
				}
				seedVertices = nextVertices;
				--burnRound;
			}
		}
		else { // only one seed in the last round
			std::cout << "multiple edges left" << std::endl;
			float burnW = -1;
			subE bestE;
			for (subE edge : burnTimeEdgeMap[burnRound]) {
				if (get(boost::edge_weight_t(), mstGraph, edge) > burnW) {
					burnW = get(boost::edge_weight_t(), mstGraph, edge);;
					bestE = edge;
				}
			}
			inverseBurnEdges.push_back(bestE);
			seedVertices.clear();
			seedVertices.push_back(bestE.m_source);
			seedVertices.push_back(bestE.m_target);
			burnRound--;
			while (burnRound > 0) {
				std::vector<int> nextVertices;
				for (int i = 0; i < seedVertices.size(); i++) {
					int bestSeed = -1;
					subE bestEdge;
					subGraph::adjacency_iterator adjIt, adjEnd;
					boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(seedVertices[i], mstGraph);
					int largestRound = -1;
					for (; adjIt != adjEnd; ++adjIt) {
						int v = *adjIt;
						bool exists;
						subE e;
						boost::tie(e, exists) = boost::edge(seedVertices[i], v, mstGraph);
						if (!exists || std::find(inverseBurnEdges.begin(), inverseBurnEdges.end(), e) != inverseBurnEdges.end())
						{
							continue;
						}
						if (edgeToBurnround[e] > largestRound) {
							if (e.m_source != seedVertices[i]) {
								bestSeed = e.m_source;
								bestEdge = e;
								largestRound = edgeToBurnround[e];
							}
							else {
								bestSeed = e.m_target;
								bestEdge = e;
								largestRound = edgeToBurnround[e];
							}
						}
					}
					if (bestSeed > -1) {
						nextVertices.push_back(bestSeed);
						inverseBurnEdges.push_back(bestEdge);
					}
				}
				seedVertices = nextVertices;
				--burnRound;
			}

		}
		std::cout << "inverBurnEdges.size is " << inverseBurnEdges.size() << std::endl;

		// topologically sort the stem
		std::vector<SkelEdge> unsorted;
		for (subE e : inverseBurnEdges)
		{
			SkelVert v0 = MSTSkelVertMap[e.m_source];
			SkelVert v1 = MSTSkelVertMap[e.m_target];
			unsorted.push_back(boost::edge(v0, v1, mSkeleton).first);
		}
		std::map<SkelVert, std::vector<SkelVert>> m;
		for (SkelEdge se : unsorted) {
			m[se.m_source].push_back(se.m_target);
			m[se.m_target].push_back(se.m_source);
		}
		std::deque<SkelVert> sequence;
		SkelVert nextSource = unsorted[0].m_target;
		SkelVert justVisited = unsorted[0].m_source;
		sequence.push_back(nextSource);
		while (m[nextSource].size() != 1) {
			SkelVert target = m[nextSource][0] != justVisited ? m[nextSource][0] : m[nextSource][1];
			sequence.push_back(target);
			justVisited = nextSource;
			nextSource = target;
		}
		nextSource = unsorted[0].m_source;
		justVisited = unsorted[0].m_target;
		sequence.push_front(nextSource);
		while (m[nextSource].size() != 1) {
			SkelVert target = m[nextSource][0] != justVisited ? m[nextSource][0] : m[nextSource][1];
			sequence.push_front(target);
			justVisited = nextSource;
			nextSource = target;
		}
		std::cout << "topologically sorted" << std::endl;

		// constrain start and end to metaNodes
		while (boost::degree(sequence.front(), mSkeleton) <= 2) {
			sequence.pop_front();
		}
		while (boost::degree(sequence.back(), mSkeleton) <= 2) {
			sequence.pop_back();
		}

		// update auto_stem and VBO
		SkelVert prev = -1;
		for (SkelVert curr : sequence) {
			if (prev != -1) {
				autoStemVBO.push_back(prev);
				autoStemVBO.push_back(curr);
				auto_stem.push_back(boost::edge(prev, curr, mSkeleton).first);
			}
			prev = curr;
		}

		// update auto_stem_metaNode
		for (SkelVert sv : sequence) {
			if (boost::degree(sv, mSkeleton) > 2) {
				MetaV v = addNode(sv, &mSkeleton);
				auto_stem_metaNode.push_back(v);
			}
		}

		// update auto_stem_metaEdge
		if (auto_stem_metaNode.size() < 2) return;
		for (size_t i = 1; i < auto_stem_metaNode.size(); ++i) {
			MetaV src = auto_stem_metaNode[i - 1];
			MetaV dst = auto_stem_metaNode[i];
			MetaE e = boost::edge(src, dst, *this).first;
			auto_stem_metaEdge.push_back(e);
		}

		selectStemStart = auto_stem_metaNode[0];
		selectStemEnd = auto_stem_metaNode[auto_stem_metaNode.size() - 1];
		selectStemStartValid = true;
		selectStemEndValid = true;

		showSuggestedStem = true;
		buildEdgeVBOs();
	}

	// input param val - look distance for each vertex
	void BMetaGraph::FindPrimaryNodeOperation(float look_distance, float kernel_bandwidth)
	{
		std::vector<MetaV> cluster_input;
		std::vector<float> cluster_input_position;
		std::deque<SkelVert> sequence;
		primary_branches.clear();
		allBranchingPointsWithParent.clear();
		allValidBranchingPoints.clear();
		branchingPointToStemNodeWithDistanceMap.clear();
		whorls.clear();
		auto_node.clear();

		std::cout << "Find primary node" << std::endl;

		if (auto_stem_metaNode.empty())
		{
			std::cout << "No stem available" << std::endl;
			return;
		}

		for (MetaV v : auto_stem_metaNode) {
			sequence.push_back(operator[](v).mSrcVert);
		}

		// Record the position of each metaNode
		std::vector<float> auto_stem_metaDist;
		float acc = 0.0f;
		auto_stem_metaDist.push_back(acc);
		for (size_t i = 1; i < auto_stem_metaNode.size(); ++i) {
			acc += operator[](boost::edge(auto_stem_metaNode[i - 1], auto_stem_metaNode[i], *this).first).mLength;
			auto_stem_metaDist.push_back(acc);
		}

		// Exclude short branches which are recognized as noises
		for (size_t i = 0; i < auto_stem_metaNode.size(); ++i)
		{
			int validBranchesOnNode = 0;
			typename graph_traits<BoostMetaGraph>::out_edge_iterator ei, ei_end;
			for (boost::tie(ei, ei_end) = out_edges(auto_stem_metaNode[i], *this); ei != ei_end; ++ei) {
				MetaV target = boost::target(*ei, *this);
				MetaV prev = (i == 0) ? -1 : auto_stem_metaNode[i - 1];
				MetaV next = (i == auto_stem_metaNode.size() - 1) ? -1 : auto_stem_metaNode[i + 1];
				if (target != prev && target != next) {
					std::cout << "Node " << auto_stem_metaNode[i] << " has a side branch to " << target;
					if (auto_stem_metaNode[i] != target) {
						std::set<MetaV> visited;
						if (branchExceedsStemBoundary(&visited, sequence, auto_stem_metaNode[i], target, 2.0f)) {
							std::cout << " that exceeds the stem boundary";
							validBranchesOnNode++;
						}
						std::cout << std::endl;
					}
				}
			}

			if (validBranchesOnNode > 0) {
				// Find the branching points
				findBranchingPointsWithParent(auto_stem_metaNode[i], sequence, 1.0f);
			}

			// Put the position values into the input vector, each repeating j times 
			// with j being the number of valid branches on the node
			for (int j = 0; j < validBranchesOnNode; ++j) {
				cluster_input_position.push_back(auto_stem_metaDist[i]);
				cluster_input.push_back(auto_stem_metaNode[i]);
			}
		}

		// Trace branches from branching points
		std::set<SkelVert> pathTaken;
		for (std::map<SkelVert, SkelVert>::iterator it = allBranchingPointsWithParent.begin(); it != allBranchingPointsWithParent.end(); ++it) {
			std::set<SkelVert> visitedWhenTracingBranchesUsingLongestPath;
			SkelVert parent = it->second;
			SkelVert branchingPoint = it->first;
			visitedWhenTracingBranchesUsingLongestPath.insert(parent);
			std::map<SkelVert, SkelVert> childToParentMap;
			std::map<SkelVert, float> pathLength;
			traceBranchUsingLongestPath(branchingPoint, sequence, &pathTaken, visitedWhenTracingBranchesUsingLongestPath, 100.0f, 50.0f, 0.6f, 10);
		}

		// Mean shift clustering
		std::vector<float> cluster = cluster_input_position;
		std::cout << "initial position" << std::endl;
		for (float position : cluster)
		{
			std::cout << position << " ";
		}
		std::cout << std::endl << std::endl << std::endl;
		int num_iterations = 20;
		for (int i = 0; i < num_iterations; ++i)
		{
			for (int j = 0; j < cluster.size(); ++j)
			{
				// for each vertex x, find the neighbouring points N(x)
				std::vector<float> neighbours = neighbourhoodPoints(cluster, cluster[j], look_distance);

				// for each vertex x, calculate mean shift m(x)
				float numerator = 0;
				float denominator = 0;
				for (float neighbour : neighbours)
				{
					float weight = gaussian_kernel(std::abs(neighbour - cluster[j]), kernel_bandwidth);
					numerator += (weight * neighbour);
					denominator += weight;
				}
				float new_position = numerator / denominator;
				cluster[j] = new_position;
			}

			for (float position : cluster)
			{
				std::cout << " " << position;
			}
			std::cout << std::endl << std::endl;
		}

		std::map<SkelVert, float> allValidBranchingPointToAutoStemMetaDistMap;
		for(SkelVert validBranchingPoint : allValidBranchingPoints) {
			MetaV stemNode = branchingPointToStemNodeWithDistanceMap[validBranchingPoint].first;
			int index = -1;
			for (int i = 0; i < (int)cluster_input.size(); ++i) {
				if (cluster_input[i] == stemNode) {
					index = i;
				}
			}
			allValidBranchingPointToAutoStemMetaDistMap[validBranchingPoint] = cluster[index];
		}

		std::map<SkelVert, float>::iterator it = allValidBranchingPointToAutoStemMetaDistMap.begin();
		while (it != allValidBranchingPointToAutoStemMetaDistMap.end()) {
			std::cout << "BranchingPoint: " << it->first << " MetaDist: " << it->second << std::endl;
			++it;
		}

		std::cout << "cluster_input.size() = " << cluster_input.size() << " cluster_input_position.size() = " << cluster_input_position.size() << std::endl;
		std::cout << "cluster.size() = " << cluster.size() << std::endl << "cluster: " << std::endl;

		for (float f : cluster) {
			std::cout << f << std::endl;
		}

		std::map<float, std::vector<MetaV>> cluster_position_to_associated_nodes_map;
		std::map<float, std::vector<MetaE>> cluster_position_to_associated_edges_map;

		MetaV prev = -1;
		for (size_t i = 0; i < cluster.size(); ++i) {
			MetaV v = cluster_input[i];
			if (v != prev) {
				cluster_position_to_associated_nodes_map[cluster[i]].push_back(v);
			}
			prev = v;
		}
		for (std::pair<float, std::vector<MetaV>> entry : cluster_position_to_associated_nodes_map) {
			std::vector<MetaV> nodes = entry.second;
			MetaV start = nodes[0];
			MetaV end = nodes[nodes.size() - 1];
			bool recording = false;
			for (size_t i = 0; i < auto_stem_metaNode.size(); ++i) {
				if (recording) {
					cluster_position_to_associated_edges_map[entry.first].push_back(boost::edge(auto_stem_metaNode[i - 1], auto_stem_metaNode[i], *this).first);
				}
				if (auto_stem_metaNode[i] == start) {
					recording = true;
				}
				if (auto_stem_metaNode[i] == end) {
					recording = false;
				}
			}
		}

		// Delete the clusters that associate to only two or less vertices
		std::map<float, int> count;
		for (float f : cluster) ++count[f];
		std::vector<float> newCluster;
		for (float f : cluster) {
			if (count[f] > 2) {
				newCluster.push_back(f);
			}
		}
		cluster = newCluster;

		// Remove duplicates
		cluster.erase(std::unique(cluster.begin(), cluster.end()), cluster.end());
		std::cout << std::endl << "cluster size: " << cluster.size() << std::endl;
		std::cout << "cluster input size: " << cluster_input_position.size() << std::endl;

		for (int i = 0; i < cluster.size(); ++i)
		{
			int branchCount = 0;
			std::cout << i << std::endl;
			// Find index according to distance
			auto low = std::lower_bound(cluster_input_position.begin(), cluster_input_position.end(), cluster[i]);
			if (low == cluster_input_position.end()) {
				low--;
			}
			float val = *low;
			std::cout << "cluster[" << i << "] = " << cluster[i] << std::endl;

			int node_index = low - cluster_input_position.begin();
			if (node_index > 0 && std::abs(cluster[i] - cluster_input_position[node_index - 1]) < std::abs(cluster[i] - cluster_input_position[node_index]))
			{
				node_index = node_index - 1;
			}

			std::map<SkelVert, float>::iterator it = allValidBranchingPointToAutoStemMetaDistMap.begin();
			while (it != allValidBranchingPointToAutoStemMetaDistMap.end()) {
				if (it->second == cluster[i]) {
					branchCount++;
				}
				++it;
			}

			std::cout << "Center Node: " << cluster_input[node_index] << std::endl;
			std::cout << "Branch Count: " << branchCount << std::endl;
			std::cout << "Associated MetaV: " << std::endl;
			for (MetaV v : cluster_position_to_associated_nodes_map[cluster[i]]) {
				std::cout << v << std::endl;
			}
			std::cout << "Associated MetaE: " << std::endl;
			for (MetaE e : cluster_position_to_associated_edges_map[cluster[i]]) {
				std::cout << e << std::endl;
			}

			// Find node according to index
			MetaV node = cluster_input[node_index];
			whorls.insert(node);
			auto_node.push_back(std::tuple<MetaV, std::vector<MetaV>, std::vector<MetaE>>(node, cluster_position_to_associated_nodes_map[cluster[i]], cluster_position_to_associated_edges_map[cluster[i]]));
		}
		std::cout << "auto_node size " << auto_node.size() << std::endl;
		if (auto_node.size() != 0) {
			showWhorl = true;
			mode = OperationMode::EditWhorl;
		}
		buildEdgeVBOs();
	}

	bool BMetaGraph::branchLongerThanThreshold(MetaV prev, MetaV self, float threshold) {
		
		// Base case: the length of the path traced exceeds the threshold
		if (threshold < 0) {
			return true;
		}

		// General case: trace down the tree and deduct each edge length along the path from the threshold (until it reaches 0)
		typename graph_traits<BoostMetaGraph>::out_edge_iterator ei, ei_end;
		for (boost::tie(ei, ei_end) = out_edges(self, *this); ei != ei_end; ++ei) {
			MetaV target = boost::target(*ei, *this);
			if (target != prev) {
				std::pair<MetaE, bool> edge_pair = boost::edge(self, target, *this);
				float distance = operator[](edge_pair.first).mLength;
				if (branchLongerThanThreshold(self, target, threshold - distance)) {
					return true;
				}
			}
		}

		// If the entire tree is traced and no path is found to be longer than the threshold, return false
		// WARNING: This is vulnerable with loops because the path length for any loop is always infinity
		return false;
	}

	bool BMetaGraph::branchExceedsStemBoundary(std::set<MetaV> *visited, std::deque<SkelVert> stem, MetaV prev, MetaV self, float timesThickness) {
		
		// Stop tracing if the current node is already visited
		if (visited->count(self)) {
			return false;
		}

		// Mark the current node as visited
		visited->insert(self);

		SkelVert srcSelf = operator[](self).mSrcVert;
		if (vertexIsOutOfRadiusRange(srcSelf, stem, timesThickness)) {
			return true;
		}
		else {
			bool atLeastOneSubtreeReturnsTrue = false;
			typename graph_traits<BoostMetaGraph>::out_edge_iterator ei, ei_end;
			for (boost::tie(ei, ei_end) = out_edges(self, *this); ei != ei_end; ++ei) {
				MetaV target = boost::target(*ei, *this);
				if (target != prev) {
					atLeastOneSubtreeReturnsTrue = atLeastOneSubtreeReturnsTrue || branchExceedsStemBoundary(visited, stem, self, target, timesThickness);
				}
			}
			return atLeastOneSubtreeReturnsTrue;
		}
	}

	BSkeleton BMetaGraph::fairSkeleton(BSkeleton skel, int iterationRound) {
		BSkeleton currSkel = skel;
		BSkeleton nextSkel = skel;
		std::cout << "Fairing " << iterationRound << " rounds." << std::endl;
		std::cout << "Fairing";
		
		for (int round = 0; round < iterationRound; ++round) {
			std::cout << ".";

			// For each metaEdge, average each of the skeleton vertices that are not on the ends with the two skeleton vertices connected to it
			metaEdgeIter mei = boost::edges(*this);
			for (; mei.first != mei.second; ++mei) {
				MetaE edge = *mei.first;
				std::vector<SkelVert> vertices = operator[](edge).mVertices;
				for (size_t i = 1; i < vertices.size() - 1; ++i) {
					SkelVert prev = vertices[i - 1];
					SkelVert curr = vertices[i];
					SkelVert next = vertices[i + 1];
					float averagedX = (currSkel.operator[](prev).x() + currSkel.operator[](curr).x() + currSkel.operator[](next).x()) / 3.0f;
					float averagedY = (currSkel.operator[](prev).y() + currSkel.operator[](curr).y() + currSkel.operator[](next).y()) / 3.0f;
					float averagedZ = (currSkel.operator[](prev).z() + currSkel.operator[](curr).z() + currSkel.operator[](next).z()) / 3.0f;
					float averagedThickness = currSkel.operator[](curr).thickness();
					float averagedWidth = currSkel.operator[](curr).width();
					Point3d averaged = Point3d(averagedX, averagedY, averagedZ, averagedThickness, averagedWidth, currSkel.operator[](curr).id);
					nextSkel.operator[](curr) = averaged;
				}
			}

			// Average each junction node with all the skeleton vertices around it
			metaVertIter mvi = boost::vertices(*this);
			for (; mvi.first != mvi.second; ++mvi) {
				MetaV vertex = *mvi.first;
				if (boost::degree(vertex, *this) != 1) {
					SkelVert curr = operator[](vertex).mSrcVert;
					int outEdgeNumber = 1;
					float xAccumulator = currSkel.operator[](curr).x();
					float yAccumulator = currSkel.operator[](curr).y();
					float zAccumulator = currSkel.operator[](curr).z();
					typename graph_traits<BoostSkeleton>::out_edge_iterator ei, ei_end;
					for (boost::tie(ei, ei_end) = out_edges(curr, currSkel); ei != ei_end; ++ei) {
						SkelVert target = boost::target(*ei, currSkel);
						xAccumulator += currSkel.operator[](target).x();
						yAccumulator += currSkel.operator[](target).y();
						zAccumulator += currSkel.operator[](target).z();
						outEdgeNumber++;
					}
					float averagedX = xAccumulator / outEdgeNumber;
					float averagedY = yAccumulator / outEdgeNumber;
					float averagedZ = zAccumulator / outEdgeNumber;
					float averagedThickness = currSkel.operator[](curr).thickness();
					float averagedWidth = currSkel.operator[](curr).width();
					Point3d averaged = Point3d(averagedX, averagedY, averagedZ, averagedThickness, averagedWidth, currSkel.operator[](curr).id);
					nextSkel.operator[](curr) = averaged;
				}
			}

			// Swap the buffer
			currSkel = nextSkel;
		}
		return currSkel;
	}

	void BMetaGraph::traceBranchUsingLongestPath(SkelVert branchingPoint, std::deque<SkelVert> stem, std::set<SkelVert> *pathTaken, std::set<SkelVert> visited, float searchDistance, float minimumBranchLength, float radiusRangeCoeff, int scoringWindowSize) {
		std::deque<std::pair<SkelVert, float>> bfsQueue;
		std::set<SkelVert> stemVertSet;
		std::set<SkelVert> pathEnds;
		std::map<SkelVert, SkelVert> childToParentMap;
		std::map<SkelVert, float> pathLength;
		for (SkelVert sv : stem) {
			stemVertSet.insert(sv);
		}
		bfsQueue.push_back(std::make_pair(branchingPoint, searchDistance));
		visited.insert(branchingPoint);
		childToParentMap[branchingPoint] = branchingPoint;
		pathLength[branchingPoint] = 0.0f;
		while (bfsQueue.size() != 0) {
			SkelVert source = bfsQueue.front().first;
			float distanceLeft = bfsQueue.front().second;
			bfsQueue.pop_front();
			if (distanceLeft < 0.0f || boost::degree(source, mSkeleton) == 1) {
				pathEnds.insert(source);
				continue;
			}
			typename graph_traits<BSkeleton>::out_edge_iterator ei, ei_end;
			for (boost::tie(ei, ei_end) = out_edges(source, mSkeleton); ei != ei_end; ++ei) {
				SkelVert target = boost::target(*ei, mSkeleton);
				if (std::find(visited.begin(), visited.end(), target) == visited.end()
					&& vertexIsOutOfRadiusRange(target, stem, radiusRangeCoeff)
					&& pathTaken->count(target) == 0) {
					visited.insert(target);
					childToParentMap[target] = source;
					SkelEdge edge = boost::edge(source, target, mSkeleton).first;
					float edgeLength = (&mSkeleton)->operator[](edge).euclidLength;
					pathLength[target] = pathLength[source] + edgeLength;
					bfsQueue.push_back(std::make_pair(target, distanceLeft - edgeLength));
				}
			}
		}
		float maxPathLength = FLT_MIN;
		for (SkelVert pathEnd : pathEnds) {
			//std::cout << "    pathEnd: " << pathEnd << " pathLength: " << pathLength[pathEnd] << std::endl;
			if (pathLength[pathEnd] > maxPathLength) {
				maxPathLength = pathLength[pathEnd];
			}
		}
		if (maxPathLength < minimumBranchLength) {
			return;
		}
		float percentLongestThreshold = 0.8 * maxPathLength;
		SkelVert bestPathBuffer = INT64_MAX;
		float scoreBuffer = FLT_MAX;
		for (SkelVert pathEnd : pathEnds) {
			if (pathLength[pathEnd] < percentLongestThreshold) {
				continue;
			}
			std::vector<SkelVert> path;
			SkelVert predecessor = childToParentMap[pathEnd];
			SkelVert self = pathEnd;
			path.push_back(pathEnd);
			while (predecessor != self) {
				path.push_back(predecessor);
				self = predecessor;
				predecessor = childToParentMap[predecessor];
			}
			if (path.size() < 3 + scoringWindowSize) {
				continue;
			}
			float scoreAccumulator = 0.0f;
			int count = path.size() - (3 + scoringWindowSize) + 1;
			for (size_t i = 0; i < count; ++i) {
				Point3d p0 = (&mSkeleton)->operator[](path[i]);
				Point3d p1 = (&mSkeleton)->operator[](path[i + 1]);
				Point3d p2 = (&mSkeleton)->operator[](path[i + 2]);
				Point3d p0_n = (&mSkeleton)->operator[](path[i + scoringWindowSize]);
				Point3d p1_n = (&mSkeleton)->operator[](path[i + 1 + scoringWindowSize]);
				Point3d p2_n = (&mSkeleton)->operator[](path[i + 2 + scoringWindowSize]);
				float firstDeriv_0_x = p0_n.x() - p0.x();
				float firstDeriv_0_y = p0_n.y() - p0.y();
				float firstDeriv_0_z = p0_n.z() - p0.z();
				float firstDeriv_1_x = p1_n.x() - p1.x();
				float firstDeriv_1_y = p1_n.y() - p1.y();
				float firstDeriv_1_z = p1_n.z() - p1.z();
				float firstDeriv_2_x = p2_n.x() - p2.x();
				float firstDeriv_2_y = p2_n.y() - p2.y();
				float firstDeriv_2_z = p2_n.z() - p2.z();
				float secondDeriv_01_x = firstDeriv_1_x - firstDeriv_0_x;
				float secondDeriv_01_y = firstDeriv_1_y - firstDeriv_0_y;
				float secondDeriv_01_z = firstDeriv_1_z - firstDeriv_0_z;
				float secondDeriv_12_x = firstDeriv_2_x - firstDeriv_1_x;
				float secondDeriv_12_y = firstDeriv_2_y - firstDeriv_1_y;
				float secondDeriv_12_z = firstDeriv_2_z - firstDeriv_1_z;
				float thirdDeriv_012_x = secondDeriv_12_x - secondDeriv_01_x;
				float thirdDeriv_012_y = secondDeriv_12_y - secondDeriv_01_y;
				float thirdDeriv_012_z = secondDeriv_12_z - secondDeriv_01_z;
				float thirdDerivMerged = std::sqrt(thirdDeriv_012_x * thirdDeriv_012_x + thirdDeriv_012_y * thirdDeriv_012_y + thirdDeriv_012_z * thirdDeriv_012_z);
				scoreAccumulator += thirdDerivMerged;
			}
			float score = scoreAccumulator / count;
			if (score < scoreBuffer) {
				scoreBuffer = score;
				bestPathBuffer = pathEnd;
			}
		}
		std::cout 
			<< "branchingPoint: " << branchingPoint 
			<< " StemNode: " << branchingPointToStemNodeWithDistanceMap[branchingPoint].first 
			<< " Distance: " << branchingPointToStemNodeWithDistanceMap[branchingPoint].second 
			<< " BestPathTo: " << bestPathBuffer 
			<< " MaxPathLength: " << maxPathLength << std::endl;
		if (bestPathBuffer == INT64_MAX) {
			return;
		}
		allValidBranchingPoints.insert(branchingPoint);
		std::vector<SkelVert> bestPath;
		SkelVert predecessor = childToParentMap[bestPathBuffer];
		SkelVert self = bestPathBuffer;
		bestPath.push_back(bestPathBuffer);
		pathTaken->insert(bestPathBuffer);
		while (predecessor != self) {
			bestPath.push_back(predecessor);
			pathTaken->insert(predecessor);
			self = predecessor;
			predecessor = childToParentMap[predecessor];
		}
		for (size_t i = 1; i < bestPath.size(); ++i) {
			traceBranchVBO.push_back(bestPath[i - 1]);
			traceBranchVBO.push_back(bestPath[i]);
		}
	}

	void BMetaGraph::findBranchingPointsWithParent(MetaV stemNode, std::deque<SkelVert> stem, float radiusRangeCoeff) {
		std::deque<SkelVert> bfsQueue;
		std::set<SkelVert> visitedNew;
		std::set<SkelVert> stemVertSet;
		std::map<SkelVert, SkelVert> childToParentMap;
		std::map<SkelVert, float> pathLength;
		for (SkelVert sv : stem) {
			stemVertSet.insert(sv);
		}
		SkelVert origin = operator[](stemNode).mSrcVert;
		bfsQueue.push_back(origin);
		visitedNew.insert(origin);
		childToParentMap[origin] = origin;
		pathLength[origin] = 0.0f;
		while (bfsQueue.size() != 0) {
			SkelVert source = bfsQueue.front();
			bfsQueue.pop_front();
			if (vertexIsOutOfRadiusRange(source, stem, radiusRangeCoeff)) {
				allBranchingPointsWithParent[source] = childToParentMap[source];
				if (branchingPointToStemNodeWithDistanceMap.count(source) == 0 || pathLength[source] < branchingPointToStemNodeWithDistanceMap[source].second) {
					branchingPointToStemNodeWithDistanceMap[source] = std::make_pair(stemNode, pathLength[source]);
				}
				continue;
			}
			typename graph_traits<BSkeleton>::out_edge_iterator ei, ei_end;
			for (boost::tie(ei, ei_end) = out_edges(source, mSkeleton); ei != ei_end; ++ei) {
				SkelVert target = boost::target(*ei, mSkeleton);
				if (visitedNew.count(target) == 0
					&& stemVertSet.count(target) == 0) {
					bfsQueue.push_back(target);
					visitedNew.insert(target);
					childToParentMap[target] = source;
					SkelEdge edge = boost::edge(source, target, mSkeleton).first;
					float edgeLength = (&mSkeleton)->operator[](edge).euclidLength;
					pathLength[target] = pathLength[source] + edgeLength;
				}
			}
		}
	}

	void BMetaGraph::traceBranch(MetaV parent, MetaV curr, std::deque<SkelVert> stem, std::set<MetaV> *visited, float windowSize, float cosineThreshold) {

		// Stop tracing if the current node is already visited
		if (visited->count(curr)) {
			return;
		}

		// Mark the current node as visited
		visited->insert(curr);

		// Param: currTailVector: the direction vector at the end of the current edge, with the end being
		//						  the target of the edge and the start being the skeleton node that is one
		//						  windowSize length above the target of the edge
		std::tuple<float, float, float> currTailVector = getDirectionVector(parent, curr, windowSize, "tail");
		typename graph_traits<BoostMetaGraph>::out_edge_iterator ei, ei_end;
		for (boost::tie(ei, ei_end) = out_edges(curr, *this); ei != ei_end; ++ei) {
			MetaV target = boost::target(*ei, *this);
			if (target != parent) {

				// Param: nextHeadVector: the direction vector at the head of the current edge, with the start being
				//						  the source of the next edge and the start being the skeleton node that is one
				//						  windowSize length below the source of the current edge
				std::tuple<float, float, float> nextHeadVector = getDirectionVector(curr, target, windowSize, "head");

				// Obtain the cosine value of the angle between currTailVector and nextHeadVector
				float vx1 = std::get<0>(currTailVector);
				float vy1 = std::get<1>(currTailVector);
				float vz1 = std::get<2>(currTailVector);
				float vx2 = std::get<0>(nextHeadVector);
				float vy2 = std::get<1>(nextHeadVector);
				float vz2 = std::get<2>(nextHeadVector);
				float dotProduct = vx1 * vx2 + vy1 * vy2 + vz1 * vz2;
				float modulus1 = std::sqrt(vx1 * vx1 + vy1 * vy1 + vz1 * vz1);
				float modulus2 = std::sqrt(vx2 * vx2 + vy2 * vy2 + vz2 * vz2);
				float cosine = dotProduct / (modulus1 * modulus2);

				// Qualifier 1: Since the part of skeleton within the stem boundary can be very messy and rugged,
				// we don't consider the angles when it's still within the stem boundary
				bool currIsWithinStemBoundary = !vertexIsOutOfRadiusRange(operator[](target).mSrcVert, stem, 1.2f);

				// Qualifier 2: However, there are some branches that are obviously noises no matter where they are.
				// Those are the ones that are short and with a target node being an endpoint.
				// Now we define short as having a length smaller than 2 * windowSize.
				bool isNoise = boost::degree(target, *this) == 1 && operator[](boost::edge(curr, target, *this).first).mLength < 2.0f * windowSize;

				// Qualifier 3: If the next edge is very long. Even if it has a weird angle at the junction, we still include it
				bool isLongEdge = operator[](boost::edge(curr, target, *this).first).mLength > 3.0f * windowSize;

				// Taking the angle and two qualifiers into account, we decide whether the edge is a valid next edge
				// for the branch we are currently tracing, and recursively trace along the edge we just added.
				if ((cosine > cosineThreshold || currIsWithinStemBoundary || isLongEdge) && !isNoise) {
					if (!currIsWithinStemBoundary) {
						primary_branches.push_back(boost::edge(curr, target, *this).first);
					}
					traceBranch(curr, target, stem, visited, windowSize, cosineThreshold);
				}
			}
		}
	}

	std::tuple<float, float, float> BMetaGraph::getDirectionVector(MetaV source, MetaV target, float windowSize, std::string headOrTail) {
		std::vector<SkelVert> subVertices = operator[](boost::edge(source, target, *this).first).mVertices;

		// If the order of the subVertices is reversed, reverse it back
		// so that the vertex at position 0 matches the source
		if (subVertices[0] == operator[](target).mSrcVert) {
			std::reverse(subVertices.begin(), subVertices.end());
		}

		// Find windowHead and windowTail according to windowSize
		// Iteratively deduct the edge length from the windowSize until the windowHead/windowTail is found
		// or it reaches the bounds of the array (when the current metaEdge has a length smaller than windowSize)
		SkelVert windowTail;
		SkelVert windowHead;
		if (headOrTail == "tail") {
			auto tempSourceIter = subVertices.end() - 2;
			auto tempTargetIter = subVertices.end() - 1;
			while (windowSize > 0 && tempSourceIter != subVertices.begin()) {
				float edgeLength = getRelativeDistance(*tempSourceIter, *tempTargetIter);
				windowSize -= edgeLength;
				tempSourceIter--;
				tempTargetIter--;
			}
			windowHead = *tempSourceIter;
			windowTail = operator[](target).mSrcVert;
		}
		else if (headOrTail == "head") {
			auto tempSourceIter = subVertices.begin();
			auto tempTargetIter = subVertices.begin() + 1;
			while (windowSize > 0 && tempTargetIter != subVertices.end() - 1) {
				float edgeLength = getRelativeDistance(*tempSourceIter, *tempTargetIter);
				windowSize -= edgeLength;
				tempSourceIter++;
				tempTargetIter++;
			}
			windowHead = operator[](source).mSrcVert;
			windowTail = *tempTargetIter;
		}

		// Calculate the three components of the vector and return it as a tuple
		float deltaX = (&mSkeleton)->operator[](windowTail).x() - (&mSkeleton)->operator[](windowHead).x();
		float deltaY = (&mSkeleton)->operator[](windowTail).y() - (&mSkeleton)->operator[](windowHead).y();
		float deltaZ = (&mSkeleton)->operator[](windowTail).z() - (&mSkeleton)->operator[](windowHead).z();
		return std::make_tuple(deltaX, deltaY, deltaZ);
	}

	std::vector<float> BMetaGraph::neighbourhoodPoints(std::vector<float> positions, float x_centroid, float distance)
	{
		std::vector<float> neighbours;
		for (float x : positions)
		{
			if (std::abs(x - x_centroid) < distance)
			{
				neighbours.push_back(x);
			}
		}
		return neighbours;
	}

	float BMetaGraph::gaussian_kernel(float distance, int bandwidth)
	{
		float val;
		val = (1 / (bandwidth*std::sqrt(2 * 3.14159))) * std::exp(-0.5*std::pow(distance / bandwidth, 2));
		return val;
	}

	void BMetaGraph::SelectStemPrimaryNodeOperation()
	{
		std::cout << "SelectStemPrimaryNodeOperation " << std::endl;
		// sort by first element of the pair in descending order
		std::sort(PrimaryNodes.begin(), PrimaryNodes.end(), [](const std::pair<int, int> &p1, const std::pair<int, int> &p2)
		{
			return (p1.first > p2.first);
		});

		NodesPredecessors.clear();
		using weight_map_t = boost::property_map<BMetaGraph, float BMetaEdge::*>::type;
		std::cout << "priamry node vector size " << PrimaryNodes.size() << std::endl;
		for (int it = 0; it < PrimaryNodes.size(); ++it)
		{
			std::cout << "iterator " << it << std::endl;
			weight_map_t kWeightMap = boost::get(&BMetaEdge::mLength, *this);
			std::vector<int> distances(boost::num_vertices(*this));
			std::vector<MetaV> predecessors(boost::num_vertices(*this));

			MetaV source = PrimaryNodes[it].second;
			boost::dijkstra_shortest_paths(*this, source,
				boost::predecessor_map(boost::make_iterator_property_map(predecessors.begin(), boost::get(boost::vertex_index, *this)))
				.distance_map(boost::make_iterator_property_map(distances.begin(), boost::get(boost::vertex_index, *this)))
				.weight_map(kWeightMap)
			);
			NodesPredecessors.push_back(PrimaryNodesPredecessorsStruct());
			NodesPredecessors[it].nodeID = it;
			NodesPredecessors[it].predecessors = predecessors;
			std::cout << "priamry node predecessors size " << predecessors.size() << std::endl;

			distances.clear();
			predecessors.clear();
		}

		buildEdgeVBOs();
	}

	void BMetaGraph::SelectPrimaryBranchesOperation()
	{
		std::cout << "Enter SelectPrimaryBranchesOperation " << std::endl;

		if (!PrimaryBranchSelectionValid)
		{
			return;
		}

		MetaV v = PrimaryBranchSelection;
		std::vector<MetaE> shourtestPath;
		for (MetaV u = CurrentPredecessors[v]; u != v; v = u, u = CurrentPredecessors[v])
		{
			std::pair<MetaE, bool> edge_pair = boost::edge(u, v, *this);
			shourtestPath.push_back(edge_pair.first);
		}

		PrimaryBranchStruct b;
		b.primaryNodeIndex = CurrentPrimaryNode;
		b.branchEndNode = PrimaryBranchSelection;
		b.metaEdges = shourtestPath;
		PrimaryBranchesObj.push_back(b);

		unselectAll();
		buildEdgeVBOs();
		std::cout << "Exit SelectPrimaryBranchesOperation " << std::endl;
	}

	void BMetaGraph::RemovePrimaryBranchesOperation()
	{
		std::cout << "Enter RemovePrimaryBranchesOperation " << std::endl;

		if (!PrimaryBranchSelectionValid)
		{
			return;
		}

		MetaV v = PrimaryBranchSelection;
		std::cout << "size " << PrimaryBranchesObj.size() << std::endl;
		for (int i = PrimaryBranchesObj.size() - 1; i >= 0; --i)
		{
			std::vector<MetaE> edges = PrimaryBranchesObj[i].metaEdges;
			if (PrimaryBranchesObj[i].primaryNodeIndex == CurrentPrimaryNode)
			{
				for (MetaE e : edges)
				{
					if (e.m_source == v || e.m_target == v)
					{
						PrimaryBranchesObj.erase(PrimaryBranchesObj.begin() + i);
						std::cout << "size " << PrimaryBranchesObj.size() << std::endl;
						break;
					}
				}
			}
		}
		std::cout << "size " << PrimaryBranchesObj.size() << std::endl;

		unselectAll();
		buildEdgeVBOs();
		std::cout << "Exit RemovePrimaryBranchesOperation " << std::endl;
	}

	void BMetaGraph::SelectSegmentPointOperation()
	{
		std::cout << "Select Segment Point Operation " << std::endl;
		if (!selectSegmentPoint1Valid || !selectSegmentPoint2Valid)
		{
			return;
		}
		SegmentPath.clear();
		SegmentPath_node.clear();
		SegmentNodesDistances.clear();

		using weight_map_t = boost::property_map<BMetaGraph, float BMetaEdge::*>::type;
		weight_map_t kWeightMap = boost::get(&BMetaEdge::mLength, *this);

		std::vector<float> distances(boost::num_vertices(*this));
		std::vector<MetaV> predecessors(boost::num_vertices(*this));

		boost::dijkstra_shortest_paths(*this, selectSegmentPoint1,
			boost::predecessor_map(boost::make_iterator_property_map(predecessors.begin(), boost::get(boost::vertex_index, *this)))
			.distance_map(boost::make_iterator_property_map(distances.begin(), boost::get(boost::vertex_index, *this)))
			.weight_map(kWeightMap)
		);

		// Extract the shortest path from start to end.
		MetaV v = selectSegmentPoint2;
		for (MetaV u = predecessors[v]; u != v; v = u, u = predecessors[v])
		{
			std::pair<MetaE, bool> edge_pair = boost::edge(u, v, *this);
			SegmentPath.push_back(edge_pair.first);
		}

		std::cout << std::endl;
		double distance = 0;
		MetaV v_tmp;
		std::cout << "selectSegmentPoint1: " << selectSegmentPoint1 << std::endl;
		std::cout << "selectSegmentPoint2: " << selectSegmentPoint2 << std::endl;
		for (std::vector<MetaE>::reverse_iterator riter = SegmentPath.rbegin(); riter != SegmentPath.rend(); ++riter)
		{
			MetaV u_tmp = boost::source(*riter, *this);
			v_tmp = boost::target(*riter, *this);
			MetaE e_tmp = boost::edge(u_tmp, v_tmp, *this).first;
			distance += operator[](e_tmp).mLength;
			std::cout << " u_tmp " << mSkeleton[operator[](u_tmp).mSrcVert].id << " -> v_tmp " << mSkeleton[operator[](v_tmp).mSrcVert].id << "    (length: " << operator[](e_tmp).mLength << ")" << std::endl;
			std::cout << "segment distance u_tmp " << u_tmp << " = boost::source (distances map): " << distances[u_tmp] << std::endl;
			std::cout << "segment distance v_tmp " << v_tmp << " = boost::target (distances map): " << distances[v_tmp] << std::endl;
			SegmentPath_node.push_back(u_tmp);
		}
		SegmentPath_node.push_back(v_tmp);
		std::cout << "segment distance from top to bottom: " << distance << std::endl;
		std::cout << "segment distance from top to bottom (distances map): " << distances[selectSegmentPoint2] << std::endl;

		// store distance map of each node on the selected segment
		// use the distance to control display radius
		for (MetaV node : SegmentPath_node)
		{
			weight_map_t weightMap = boost::get(&BMetaEdge::mLength, *this);

			std::vector<float> distancemap(boost::num_vertices(*this));
			std::vector<MetaV> predecessormap(boost::num_vertices(*this));

			boost::dijkstra_shortest_paths(*this, node,
				boost::predecessor_map(boost::make_iterator_property_map(predecessormap.begin(), boost::get(boost::vertex_index, *this)))
				.distance_map(boost::make_iterator_property_map(distancemap.begin(), boost::get(boost::vertex_index, *this)))
				.weight_map(weightMap)
			);
			SegmentNodesDistanceStruct info;
			info.node = node;
			info.distance = distancemap;
			SegmentNodesDistances.push_back(info);
		}
		SetSelectSegmentPointOperation();
		unselectAll();
		return;
	}

	void BMetaGraph::SetSelectSegmentPointOperation()
	{
		if (SegmentPath.empty() || SegmentPath_node.empty())
		{
			return;
		}

		// selected segment. find all edges. add to vector
		std::vector<bool> visitedNodes = std::vector<bool>(mSkeleton.m_vertices.size(), false);
		std::vector<MetaV> SegmentMetaNodeStack = {};
		SegmentMetaEdges.clear();
		SegmentMetaNodes.clear();

		// add all metaEdges on stem to VBO
		// mark all metaNodes on stem to visited
		for (MetaE edge : SegmentPath)
		{
			SegmentMetaEdges.push_back(edge);
		}
		std::cout << "num of edges on selected segment: " << SegmentMetaEdges.size() << std::endl;
		for (MetaV node : SegmentPath_node)
		{
			visitedNodes[node] = true;
			SegmentMetaNodeStack.push_back(node);
		}
		SegmentMetaNodeStack.erase(SegmentMetaNodeStack.begin());
		SegmentMetaNodeStack.pop_back();

		std::cout << "num of nodes on segment stem: " << SegmentMetaNodeStack.size() << std::endl;

		// find all adjacent MetaNode of selectSegmentPoint1 and selectSegmentPoint2
		// mark these metaNode as visited. limit searching range
		BMetaGraph::adjacency_iterator adjIt, adjEnd;
		std::deque<MetaV> neighbors;
		neighbors.push_back(selectSegmentPoint1);
		neighbors.push_back(selectSegmentPoint2);
		visitedNodes[selectSegmentPoint1] = true;
		visitedNodes[selectSegmentPoint2] = true;
		// prevent loops effect
		for (int i = 0; i < 80; i++)
		{
			if (neighbors.empty())
				break;
			MetaV node = neighbors.front();
			neighbors.pop_front();
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(node, *this);
			for (; adjIt != adjEnd; ++adjIt)
			{
				MetaV temp = *adjIt;
				if (!visitedNodes[temp])
				{
					neighbors.push_back(temp);
					visitedNodes[temp] = true;
				}
			}
		}
		// breadth first search and add metaEdges within user defined range to VBO
		while (!SegmentMetaNodeStack.empty())
		{
			BMetaGraph::adjacency_iterator adjIt, adjEnd;
			MetaV currentNode = SegmentMetaNodeStack.front();
			SegmentMetaNodeStack.erase(SegmentMetaNodeStack.begin());
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(currentNode, *this);

			for (; adjIt != adjEnd; ++adjIt)
			{
				MetaE e;
				bool exists;
				boost::tie(e, exists) = boost::edge(currentNode, *adjIt, *this);
				if (exists && MinDistanceToSelectedSegment(*adjIt) < SegmentHorizontalRadius)
				{
					// distance less than SegmentHorizontalRadius
					SegmentMetaEdges.push_back(e);
					if (!visitedNodes[*adjIt])
					{
						visitedNodes[*adjIt] = true;
						SegmentMetaNodeStack.push_back(*adjIt);
					}
				}
			}
		}

		for (MetaE e : SegmentMetaEdges)
		{
			MetaV v0 = e.m_source;
			if (std::find(SegmentMetaNodes.begin(), SegmentMetaNodes.end(), v0) == SegmentMetaNodes.end())
			{
				SegmentMetaNodes.push_back(v0);
			}
			MetaV v1 = e.m_target;
			if (std::find(SegmentMetaNodes.begin(), SegmentMetaNodes.end(), v1) == SegmentMetaNodes.end())
			{
				SegmentMetaNodes.push_back(v1);
			}
		}
		buildEdgeVBOs();
		std::cout << "Found segment " << std::endl;
		return;

	}

	float BMetaGraph::MinDistanceToSelectedSegment(MetaV point)
	{
		float distance = FLT_MAX;
		for (int i = 0; i < SegmentNodesDistances.size(); i++)
		{
			if (distance > SegmentNodesDistances[i].distance[point])
			{
				distance = SegmentNodesDistances[i].distance[point];
			}
		}
		return distance;
	}

	float BMetaGraph::DistanceFromPointToLine(MetaV point, MetaV line1, MetaV line2)
	{
		float distance = 0;
		float p[3] = { operator[](point).x(), operator[](point).y(), operator[](point).z() };
		float p1[3] = { operator[](line1).x(), operator[](line1).y(), operator[](line1).z() };
		float p2[3] = { operator[](line2).x(), operator[](line2).y(), operator[](line2).z() };
		/* distance
		https://math.stackexchange.com/questions/1300484/distance-between-line-and-a-point
		*/
		float* a = Substract3DPoint(p, p1);
		float* b = Substract3DPoint(p2, p1);
		float c, d, e;
		c = 0; d = 0; e = 0;
		for (int i = 0; i < 3; ++i)
		{
			c = a[i] * b[i] + c;
			d = b[i] * b[i] + d;
		}
		e = c / d;

		for (int i = 0; i < 3; ++i)
		{
			b[i] = e * b[i];
		}
		float* distVector = Substract3DPoint(a, b);
		for (int i = 0; i < 3; ++i)
		{
			distance = distVector[i] * distVector[i] + distance;
		}
		distance = std::sqrt(distance);
		return distance;
	}

	float* BMetaGraph::Substract3DPoint(float *point0, float *point1)
	{
		float* out = new float[5];
		for (int i = 0; i < 3; ++i)
			out[i] = point0[i] - point1[i];
		return out;
	}

	void BMetaGraph::PromoteOperation(SkelVert toPromote)
	{
		std::cout << "Promote operation triggered " << std::endl;
		metaEdgeIter mei = boost::edges(*this);
		bool edgeFound = false;
		for (; mei.first != mei.second; ++mei)
		{
			BMetaEdge *edge = &operator[](*mei.first);

			for (SkelVert v : edge->mVertices)
			{
				if (v == toPromote)
				{
					edgeFound = true;
					break;
				}
			}
			if (edgeFound)
			{
				MetaV leftNode = vertNodeMap[edge->start()];
				MetaV rightNode = vertNodeMap[edge->end()];
				MetaV myNode = addNode(toPromote, &mSkeleton);
				std::pair<BMetaEdge, BMetaEdge> split = edge->split(toPromote, &mSkeleton);
				MetaE leftEdge, rightEdge;
				bool success = false;
				boost::tie(leftEdge, success) = add_edge(leftNode, myNode, *this);
				if (success)
				{
					operator[](leftEdge) = split.first;
				}
				boost::tie(rightEdge, success) = add_edge(myNode, rightNode, *this);
				if (success)
				{
					operator[](rightEdge) = split.second;
				}

				updateNodeDegrees();
				removeEdgeNoBridging(*mei.first);

				break;
			}
		}
	}

	bool BMetaGraph::BridgeNode(MetaV nodeToBridge)
	{
		std::cout << "Beginning to bridge " << std::endl;
		if (nodeToBridge >= m_vertices.size())
		{
			std::cout << "Invalid index " << std::endl;
			return false;
		}
		if (boost::degree(nodeToBridge, *this) == 2)
		{
			std::cout << "Node should be bridged " << std::endl;
			BMetaGraph::adjacency_iterator adjIt, adjEnd;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(nodeToBridge, *this);
			MetaV v1 = *adjIt;
			MetaV v2 = *(adjIt + 1);
			std::cout << "v1 " << v1 << " v2 " << v2 << std::endl;
			MetaE e1, e2;
			bool exists = false;
			boost::tie(e1, exists) = boost::edge(nodeToBridge, v1, *this);
			boost::tie(e2, exists) = boost::edge(nodeToBridge, v2, *this);
			if (v1 == nodeToBridge || v2 == nodeToBridge)
			{
				std::cout << "bridging to self - do nothing" << std::endl;
				std::cout << "self edge length " << operator[](e1).mLength << std::endl;
				return false;
			}
			BMetaEdge joinedEdge = operator[](e1).join(operator[](e2), &mSkeleton);
			std::cout << "adding edge " << std::endl;
			MetaE addedEdge = addEdge(v1, v2, joinedEdge.mVertices, &mSkeleton, false);
			std::cout << "succeeded in adding edge " << std::endl;
			boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(nodeToBridge, *this);
			std::vector<MetaE> toRemove = {};
			if (v1 == v2)
			{
				std::cout << "self edge info " << std::endl;
				std::cout << "length " << operator[](addedEdge).mLength << std::endl;
				std::cout << "start vert " << addedEdge.m_source << " end vert " << addedEdge.m_target << std::endl;
				removeEdge(e1);
				MetaE edge;
				bool exists = false;
				boost::tie(edge, exists) = boost::edge(nodeToBridge, v1, *this);
				if (exists)
				{
					removeEdge(edge);
				}

			}
			else
			{
				for (; adjIt != adjEnd; ++adjIt)
				{
					MetaE edge;
					boost::tie(edge, exists) = boost::edge(nodeToBridge, *adjIt, *this);

					if (exists)
					{
						toRemove.push_back(edge);
					}
				}
				for (MetaE edge : toRemove)
				{
					removeEdge(edge);
				}
			}


			removeNode(nodeToBridge);
			findAndLabelConnectedComponents();
			operator[](addedEdge).updateColors(edgeOptions, vertexColors, &mSkeleton);

			return true;
		}
		return false;
	}

	int BMetaGraph::GetEdgeComponent(MetaE edge)
	{
		BMetaEdge src = operator[](edge);
		//ensure that both endpoint vertices are mapped as metanodes

		if (vertNodeMap.count(src.start()) == 0 ||
			vertNodeMap.count(src.end()) == 0)
		{
			//std::cout << "The endpoints of this metaedge are not mapped as metanodes" << std::endl;
			return -1;
		}

		MetaV node1 = vertNodeMap[src.start()];
		MetaV node2 = vertNodeMap[src.end()];

		int component1 = operator[](node1).connectedComponent;
		int component2 = operator[](node2).connectedComponent;

		if (component1 != component2)
		{
			ConnectComponents(component1, component2);
		}

		return operator[](node1).connectedComponent;
	}

	void BMetaGraph::bridgeUtil(int u, bool visited[], int disc[],
		int low[], int parent[], std::vector<std::pair<MetaV, MetaV>> &bridgeNodes)
	{
		// A static variable is used for simplicity, we can 
		// avoid use of static variable by passing a pointer.
		static int time = 0;

		// Mark the current node as visited
		visited[u] = true;

		// Initialize discovery time and low value
		disc[u] = low[u] = ++time;

		// Go through all vertices aadjacent to this
		BMetaGraph::adjacency_iterator adjIt, adjEnd;
		boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(u, *this);

		for (; adjIt != adjEnd; ++adjIt)
		{
			size_t v = *adjIt;

			if (!visited[v])
			{
				parent[v] = u;
				bridgeUtil(v, visited, disc, low, parent, bridgeNodes);

				low[u] = std::min(low[u], low[v]);

				if (low[v] > disc[u])
				{
					bridgeNodes.push_back(std::make_pair(u, v));
					//std::cout << u << " " << v << std::endl;
				}
			}
			else if (v != parent[u])
			{
				low[u] = std::min(low[u], disc[v]);
			}
		}
	}

	// DFS based function to find all bridges. It uses recursive 
	// function bridgeUtil()
	void BMetaGraph::findBridges()
	{
		// Mark all the vertices as not visited
		bool *visited = new bool[m_vertices.size()];
		int *disc = new int[m_vertices.size()];
		int *low = new int[m_vertices.size()];
		int *parent = new int[m_vertices.size()];

		// Initialize parent and visited arrays
		for (int i = 0; i < m_vertices.size(); i++)
		{
			parent[i] = -1;
			visited[i] = false;
		}

		metaEdgeIter mei = boost::edges(*this);
		for (; mei.first != mei.second; ++mei)
		{
			operator[](*mei.first).isBridge = false;
		}

		std::vector<std::pair<MetaV, MetaV>> bridgeNodes = {};

		std::cout << "==================Bridges exist between the following vertices==================" << std::endl;
		// Call the recursive helper function to find Bridges
		// in DFS tree rooted with vertex 'i'
		for (int i = 0; i < m_vertices.size(); i++)
			if (visited[i] == false)
				bridgeUtil(i, visited, disc, low, parent, bridgeNodes);


		for each(auto nodepair in bridgeNodes)
		{
			MetaE e;
			bool exists;
			boost::tie(e, exists) = boost::edge(nodepair.first, nodepair.second, *this);

			if (exists)
			{
				operator[](e).isBridge = true;
			}
		}

		delete[] visited;
		delete[] disc;
		delete[] low;
		delete[] parent;
		std::cout << "==================End Bridges==================" << std::endl;
	}

	//////////////////////////////// Disjoint Sets ////////////////////////////////

	// Constructor. 
	DisjointSets::DisjointSets(int n)
	{
		// Allocate memory 
		this->n = n;
		parent = new int[n + 1];
		rnk = new int[n + 1];

		// Initially, all vertices are in 
		// different sets and have rank 0. 
		for (int i = 0; i <= n; i++)
		{
			rnk[i] = 0;

			//every element is parent of itself 
			parent[i] = i;
		}
	}

	// Find the parent of a node 'u' 
	// Path Compression 
	int DisjointSets::find(int u)
	{
		// Make the parent of the nodes in the path from u--> parent[u] point to parent[u] 
		if (u != parent[u])
			parent[u] = find(parent[u]);
		return parent[u];
	}

	// Union by rank 
	void DisjointSets::merge(int x, int y)
	{
		x = find(x), y = find(y);

		// Make tree with smaller height a subtree of the other tree
		if (rnk[x] > rnk[y])
			parent[y] = x;
		else // If rnk[x] <= rnk[y] 
			parent[x] = y;

		if (rnk[x] == rnk[y])
			rnk[y]++;
	}

}