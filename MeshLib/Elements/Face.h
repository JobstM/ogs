/**
 * Copyright (c) 2012, OpenGeoSys Community (http://www.opengeosys.com)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.com/LICENSE.txt
 *
 *
 * \file Face.h
 *
 * Created on 2012-05-02 by Karsten Rink
 */

#ifndef FACE_H_
#define FACE_H_

#include "Element.h"


namespace MeshLib {

/**
 * Virtual base class for 2d mesh elements.
 */
class Face : public Element
{
public:
	/// Get the area of this 2d element.
	virtual double getArea() const { return _area; };

	/// Returns the length, area or volume of a 1D, 2D or 3D element
	double getContent() const { return _area; };

	/// Get dimension of the mesh element.
	unsigned getDimension() const { return 2; };

	/// Returns the face i of the element.
	const Element* getFace(unsigned i) const { return this->getEdge(i); };

	/// Get the number of nodes for face i.
	unsigned getNFaceNodes(unsigned i) const { (void)i; return 2; };

	/// 2D elements have no faces.
	unsigned getNFaces() const { return 0; };

	/// Returns the surface normal of a 2D element.
	void getSurfaceNormal(double normal[3]) const;

	/// Destructor
	virtual ~Face();

	/**
	 * This method is pure virtual and is inherited from class @sa Element.
	 * It has to be implemented in the derived classes of class Face!
	 * @return a copy of the object
	 */
	virtual Element* clone() const = 0;

protected:
/*
	/// Constructor for a generic mesh element containing an array of mesh nodes.
	Face(Node** nodes, MshElemType::type type, unsigned value = 0);
*/
	/// Constructor for a generic mesh element without an array of mesh nodes.
	Face(unsigned value = 0);

	double _area;

private:


}; /* class */

} /* namespace */

#endif /* FACE_H_ */

