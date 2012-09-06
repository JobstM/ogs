/**
 * Copyright (c) 2012, OpenGeoSys Community (http://www.opengeosys.com)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.com/LICENSE.txt
 *
 *
 * \file Tri.h
 *
 * Created on 2012-05-02 by Karsten Rink
 */

#ifndef TRI_H_
#define TRI_H_

#include "Face.h"

namespace MeshLib {

/**
 * This class represents a 2d triangle element. The following sketch shows the node and edge numbering.
 * @anchor TriNodeAndEdgeNumbering
 * @code
 *
 *          2
 *          o
 *         / \
 *        /   \
 *      2/     \1
 *      /       \
 *     /         \
 *    0-----------1
 *          0
 *
 * @endcode
 */
class Tri : public Face
{
public:
	/// Constructor with an array of mesh nodes.
	Tri(Node* nodes[3], unsigned value = 0);

	/// Constructor using single mesh nodes.
	Tri(Node* n0, Node* n1, Node* n2, unsigned value = 0);

	/// Copy constructor
	Tri(const Tri &tri);

	/// Destructor
	virtual ~Tri();

	/// Get the number of edges for this element.
	unsigned getNEdges() const { return 3; };

	/// Get the number of neighbors for this element.
	unsigned getNNeighbors() const { return 3; };

	/// Get the number of nodes for this element.
	virtual unsigned getNNodes() const { return 3; };

	/**
	 * Method returns the type of the element. In this case TRIANGLE will be returned.
	 * @return MshElemType::TRIANGLE
	 */
	virtual MshElemType::type getType() const { return MshElemType::TRIANGLE; }

	/**
	 * Method clone is inherited from class Element. It makes a deep copy of the Tri instance.
	 * @return an exact copy of the object
	 */
	virtual Element* clone() const;

	/**
	 * This method should be called after at least two nodes of the triangle
	 * element are collapsed. As a consequence of the node collapsing an edge
	 * of the triangle will be collapsed. If one of the edges is collapsed we
	 * obtain an edge. In this case the method will create the appropriate
	 * object of class Edge.
	 * @return an Edge object or NULL
	 */
	virtual Element* reviseElement() const;

protected:
	/// Calculates the area of the triangle by returning half of the area of the corresponding parallelogram.
	double computeVolume();

protected:
	/// Return a specific edge node.
	inline Node const* getEdgeNode(unsigned edge_id, unsigned node_id) const { return _nodes[_edge_nodes[edge_id][node_id]]; };

	/// Returns the ID of a face given an array of nodes.
	unsigned identifyFace(Node* nodes[3]) const;

	static const unsigned _edge_nodes[3][2];

}; /* class */

} /* namespace */

#endif /* TRI_H_ */

