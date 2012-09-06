/**
 * Copyright (c) 2012, OpenGeoSys Community (http://www.opengeosys.net)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.net/LICENSE.txt
 *
 * \file MshLayerMapper.cpp
 *
 * Created on 2010-11-01 by Karsten Rink
 */

#include "MshLayerMapper.h"
#include "VtkRaster.h"

#include "Mesh.h"
#include "Node.h"
#include "Elements/Element.h"
#include "Elements/Hex.h"
#include "Elements/Prism.h"
#include "MshEditor.h"
#include "MathTools.h"

#include <QImage>

MeshLib::Mesh* MshLayerMapper::CreateLayers(const MeshLib::Mesh* mesh, size_t nLayers, double thickness)
{
	if (nLayers < 1 || thickness <= 0 || mesh->getDimension() != 2)
	{
		std::cout << "Error in MshLayerMapper::CreateLayers() - A 2D mesh with nLayers>0 and thickness>0 is required as input." << std::endl;
		return NULL;
	}

	const size_t nNodes = mesh->getNNodes();
	const size_t nElems = mesh->getNElements();
	const std::vector<MeshLib::Node*> nodes = mesh->getNodes();
	const std::vector<MeshLib::Element*> elems = mesh->getElements();
	std::vector<MeshLib::Node*> new_nodes(nNodes + (nLayers * nNodes));
	std::vector<MeshLib::Element*> new_elems(nElems * nLayers);

	for (size_t layer_id = 0; layer_id <= nLayers; layer_id++)
	{
		// add nodes for new layer
		unsigned node_offset (nNodes * layer_id);
		const double z_offset (layer_id * thickness);
		for (size_t i = 0; i < nNodes; i++)
		{
			const double* coords = nodes[i]->getCoords();
			new_nodes[i] = new MeshLib::Node(coords[0], coords[1], coords[2]-z_offset, node_offset+i);
		}

		// starting with 2nd layer create prism or hex elements connecting the last layer with the current one
		if (layer_id > 0)
		{
			node_offset -= nNodes;
			const unsigned mat_id (nLayers - layer_id);

			for (size_t i = 0; i < nElems; i++)
			{
				const MeshLib::Element* sfc_elem( elems[i] );
				if (sfc_elem->getDimension() == 2)
				{
					const unsigned nElemNodes(sfc_elem->getNNodes());
					MeshLib::Node** e_nodes = new MeshLib::Node*[2*nElemNodes];
					
					for (size_t j=0; j<nElemNodes; j++)
					{
						const unsigned node_id = sfc_elem->getNode(j)->getID() + node_offset;
						e_nodes[j] = new_nodes[node_id];
						e_nodes[j+nElemNodes] = new_nodes[node_id+nNodes];
					}
					if (sfc_elem->getType() == MshElemType::TRIANGLE)	// extrude triangles to prism
						new_elems.push_back(new MeshLib::Prism(e_nodes, mat_id));
					else if (sfc_elem->getType() == MshElemType::QUAD)	// extrude quads to hexes
						new_elems.push_back(new MeshLib::Hex(e_nodes, mat_id));

					delete e_nodes;
				}
				else
				{
					std::cout << "Warning in MshLayerMapper::CreateLayers() - Method can only handle 2D mesh elements ..." << std::endl;
					std::cout << "Skipping Element " << i << " of type \"" << MshElemType2String(sfc_elem->getType()) << "\"." << std::endl;
				}
			}
		}
	}

	return new MeshLib::Mesh("NewMesh", new_nodes, new_elems);
}

int MshLayerMapper::LayerMapping(MeshLib::Mesh* new_mesh, const std::string &rasterfile,
                                 const size_t nLayers, const size_t layer_id, bool removeNoDataValues)
{
	if (new_mesh == NULL)
	{
		std::cout <<
		"Error in MshLayerMapper::LayerMapping() - Passed Mesh is NULL..." <<
		std::endl;
		return 0;
	}

	if (nLayers >= layer_id)
	{
		double x0(0), y0(0), delta(1);
		size_t width(1), height(1);
		float* elevation = VtkRaster::loadDataFromASC(rasterfile, x0, y0, width,height, delta);

		if (elevation == NULL)
		{
			delete [] elevation;
			return 0;
		}

		const std::pair<double, double> xDim(x0, x0 + width * delta); // extension in x-dimension
		const std::pair<double, double> yDim(y0, y0 + height * delta); // extension in y-dimension

		if (!meshFitsImage(new_mesh, xDim, yDim))
		{
			delete [] elevation;
			return 0;
		}

		const size_t nNodes = new_mesh->getNNodes();
		const size_t nNodesPerLayer = nNodes / (nLayers+1);

		const size_t firstNode = layer_id * nNodesPerLayer;
		const size_t lastNode  = firstNode + nNodesPerLayer;

		std::vector<size_t> noData_nodes;
		const double half_delta = 0.5*delta;
		const std::vector<MeshLib::Node*> nodes = new_mesh->getNodes();
		for(size_t i = firstNode; i < lastNode; i++)
		{
			const double* coords = nodes[i]->getCoords();
			// position in raster
			const double xPos ((coords[0] - xDim.first) / delta);
			const double yPos ((coords[1] - yDim.first) / delta);
			// raster cell index
			const size_t xIdx (static_cast<size_t>(floor(xPos)));
			const size_t yIdx (static_cast<size_t>(floor(yPos)));

			// deviation of mesh node from centre of raster cell ( in [-1:1) because it is normalised by delta/2 )
			const double xShift = (xPos-xIdx-half_delta)/half_delta;
			const double yShift = (yPos-yIdx-half_delta)/half_delta;

			const int xShiftIdx = (xShift>=0) ? ceil(xShift) : floor(xShift);
			const int yShiftIdx = (yShift>=0) ? ceil(yShift) : floor(yShift);

			// determining the neighbouring pixels that add weight to the interpolation
			const size_t x_nb[4] = {0, xShiftIdx, xShiftIdx, 0};
			const size_t y_nb[4] = {0, 0, yShiftIdx, yShiftIdx};

			double locZ[4];
			locZ[0] = elevation[2*(yIdx*width + xIdx)];
			if (fabs(locZ[0] + 9999) > std::numeric_limits<double>::min())
			{
				for (size_t j=1; j<4; j++)
				{
					locZ[j] = elevation[2*((yIdx+y_nb[j])*width + (xIdx+x_nb[j]))];
					if (fabs(locZ[j] + 9999) < std::numeric_limits<double>::min())
						locZ[j]=locZ[0];
				}

				double ome[4];
				double xi = 1-fabs(xShift);
				double eta = 1-fabs(xShift);
				MathLib::MPhi2D(ome, xi, eta);

				double z(0.0);
				for(size_t j = 0; j < 4; j++)
					z += ome[j] * locZ[j];
				const double* coords (nodes[i]->getCoords());
				nodes[i]->updateCoordinates(coords[0], coords[1], z);
				//nodes[i]->SetMark(true);
			}
			else
			{
				const double* coords (nodes[i]->getCoords());
				nodes[i]->updateCoordinates(coords[0], coords[1], 0);
				//nodes[i]->SetMark(false);
				noData_nodes.push_back(i);
			}
		}

		if ((nLayers == 0) && removeNoDataValues)
		{
			if (noData_nodes.size() < (nNodes - 2))
			{
				std::cout << "Warning: Removing " << noData_nodes.size() 
					      << " mesh nodes at NoData values ... " << std::endl;
				MeshLib::MshEditor msh_editor;
				MeshLib::Mesh* red_mesh = msh_editor.removeMeshNodes(new_mesh, noData_nodes);
				if (new_mesh->getNElements() == 0)
				{
					delete new_mesh;
					new_mesh = red_mesh;
				}
				else
				{
					delete red_mesh;
					std::cout << "Too many NoData values..." << std::endl;
				}
			}
			else
				std::cout << "Too many NoData values..." << std::endl;
		}

		delete [] elevation;
		return 1;
	}
	else
		std::cout << "Error in MshLayerMapper::LayerMapping() - Mesh has only " 
		          << nLayers << " Layers, cannot assign layer " << layer_id 
				  << "..." << std::endl;
	return 0;
}

bool MshLayerMapper::meshFitsImage(const MeshLib::Mesh* msh,
                                   const std::pair<double, double> &xDim,
                                   const std::pair<double, double> &yDim)
{
	const size_t nNodes = msh->getNNodes();
	const std::vector<MeshLib::Node*> nodes = msh->getNodes();
	const double* pnt;
	double xMin(std::numeric_limits<double>::max());
	double yMin(std::numeric_limits<double>::max());
	double xMax(std::numeric_limits<double>::min());
	double yMax(std::numeric_limits<double>::min());
		
	for (size_t i = 1; i < nNodes; i++)
	{
		pnt = nodes[i]->getCoords();
		if (xMin > pnt[0])
			xMin = pnt[0];
		else if (xMax < pnt[0])
			xMax = pnt[0];

		if (yMin > pnt[1])
			yMin = pnt[1];
		else if (yMax < pnt[1])
			yMax = pnt[1];
	}

	if (xMin < xDim.first || xMax > xDim.second || yMin < yDim.first || yMax > yDim.second)
	{
		std::cout << "Warning: Mesh does not fit into given raster file." << std::endl;
		return false;
	}
	return true;
}

MeshLib::Mesh* MshLayerMapper::blendLayersWithSurface(MeshLib::Mesh* mesh, const size_t nLayers, const std::string &dem_raster)
{
/*
	// construct surface mesh from DEM
	MeshLib::Mesh* dem = MshEditor::getMeshSurface(*mesh);
	MshLayerMapper::LayerMapping(dem, dem_raster, 0, 0);

	const size_t nNodes = mesh->nod_vector.size();
	const size_t nNodesPerLayer = nNodes / (nLayers+1);
	std::vector<bool> is_surface_node(nNodes, false);
	std::vector<bool> nodes_below_surface(nNodes, false);

	// check if bottom layer nodes are below DEM
	const size_t bottom_firstNode = nLayers * nNodesPerLayer;
	const size_t bottom_lastNode  = bottom_firstNode + nNodesPerLayer;
	for(size_t i = bottom_firstNode; i < bottom_lastNode; i++)
	{
		nodes_below_surface[i]=true;
		const double* coords = mesh->nod_vector[i]->getData();
		const double* dem_coords = dem->nod_vector[i-bottom_firstNode]->getData();
		if (coords[2] >= dem_coords[2])
		{
			std::cout << "Warning: Node " << i << " (in bottom-layer) is above surface node " << (i-bottom_firstNode) << ". (" << coords[2] << " > " << dem_coords[2] << ")" << std::endl;
			is_surface_node[i] = true;
		}
	}

	// for all other layers:
	// if node < dem-node: do nothing
	// if node > dem-node:
	//		if first node above surface: map to dem and mark as surface node
	//		else remove node
	for (int layer_id=nLayers-1; layer_id>=0; layer_id--)
	{
		const size_t firstNode = layer_id * nNodesPerLayer;
		const size_t lastNode  = firstNode + nNodesPerLayer;

		for(size_t i = firstNode; i < lastNode; i++)
		{
			if (is_surface_node[i+nNodesPerLayer])
				is_surface_node[i]=true;
			else
			{
				nodes_below_surface[i]=true;
				MeshLib::CNode* node (mesh->nod_vector[i]);
				const double* coords = node->getData();
				const double* dem_coords = dem->nod_vector[i-firstNode]->getData();
				if (coords[2] > dem_coords[2])
				{
					const double new_coords[3] = { dem_coords[0], dem_coords[1], dem_coords[2] };
					node->SetCoordinates(new_coords);
					is_surface_node[i] = true;
				}
			}
		}
	}

	std::vector<GeoLib::Point*> *nodes = new std::vector<GeoLib::Point*>;
	std::vector<int> node_index_map(nNodes, -1);
	size_t node_count(0);
	for (size_t j=0; j<nNodes; j++)
	{
		if (nodes_below_surface[j])
		{
			nodes->push_back(new GeoLib::Point(mesh->nod_vector[j]->getData()));
			node_index_map[j]=node_count++;
		}
	}

	const size_t nElems = mesh->ele_vector.size();
	std::vector<GridAdapter::Element*> *elements = new std::vector<GridAdapter::Element*>;
	for (size_t j=0; j<nElems; j++)
	{
		const MeshLib::CElem* elem = mesh->ele_vector[j];

		size_t count(0);
		for (size_t i=0; i<6; i++) // check top surface of prism
			if (nodes_below_surface[elem->GetNodeIndex(i)]) count++;

		if (count==6) // copy prism elements if all six nodes are valid
		{
			GridAdapter::Element* prism = new GridAdapter::Element;
			std::vector<size_t> elem_nodes;
			for (size_t i=0; i<6; i++)
				elem_nodes.push_back( node_index_map[elem->GetNodeIndex(i)] );
			prism->material = elem->GetPatchIndex();
			prism->type = MshElemType::PRISM;
			prism->nodes = elem_nodes;
			elements->push_back(prism);
		}
		else if (count==5) // change the current element to two tetrahedra if only five nodes are valid
		{
			GridAdapter::Element* tet1 = new GridAdapter::Element;
			std::vector<size_t> elem_nodes;
			if (nodes_below_surface[elem->GetNodeIndex(0)])
				elem_nodes.push_back( node_index_map[elem->GetNodeIndex(0)] );
			else
				elem_nodes.push_back( node_index_map[elem->GetNodeIndex(1)] );
			for (size_t i=3; i<6; i++)
				elem_nodes.push_back( node_index_map[elem->GetNodeIndex(i)] );
			tet1->material = elem->GetPatchIndex();
			tet1->type = MshElemType::TETRAHEDRON;
			tet1->nodes = elem_nodes;
			elements->push_back(tet1);

			GridAdapter::Element* tet2 = new GridAdapter::Element;
			std::vector<size_t> elem_nodes2;
			if (nodes_below_surface[elem->GetNodeIndex(0)])
			{
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(0)] );
				if (nodes_below_surface[elem->GetNodeIndex(1)])
					elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(1)] );
				else
					elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(2)] );
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(5)] );
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(4)] );
			}
			else
			{
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(1)] );
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(2)] );
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(3)] );
				elem_nodes2.push_back( node_index_map[elem->GetNodeIndex(5)] );
			}
			tet2->material = elem->GetPatchIndex();
			tet2->type = MshElemType::TETRAHEDRON;
			tet2->nodes = elem_nodes2;
			elements->push_back(tet2);
		}
		else if (count==4) // change the current element to a tetrahedron if only four nodes are valid
		{
			std::vector<size_t> elem_nodes;
			for (size_t i=0; i<3; i++)
				if (nodes_below_surface[elem->GetNodeIndex(i)])
					elem_nodes.push_back( node_index_map[elem->GetNodeIndex(i)] );

			if (elem_nodes.size()==1) // make sure than only one node is from the upper layer and three from the lower
			{
				for (size_t i=3; i<6; i++)
					elem_nodes.push_back( node_index_map[elem->GetNodeIndex(i)] );
				GridAdapter::Element* tet = new GridAdapter::Element;
				tet->material = elem->GetPatchIndex();
				tet->type = MshElemType::TETRAHEDRON;
				tet->nodes = elem_nodes;
				elements->push_back(tet);
			}
		}
		// else remove element, if less than four nodes are valid
	}
	GridAdapter grid;
	grid.setNodeVector(nodes);
	grid.setElements(elements);
	MeshLib::CFEMesh* struct_mesh = new MeshLib::CFEMesh(*grid.getCFEMesh());
	return struct_mesh;
*/
	return new MeshLib::Mesh(*mesh);
}



