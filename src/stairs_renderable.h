/* Copyright 2012 Brian Ellis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef STAIRS_RENDERABLE_H
#define STAIRS_RENDERABLE_H

#include "enums.h"
#include "renderable.h"

#include <QPair>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

class StairsRenderable : public Renderable {
 public:
  explicit StairsRenderable(const QVector3D& size);
  virtual ~StairsRenderable () {}

  virtual void initialize();

  virtual void renderAt(const QVector3D& location, const BlockOrientation* orientation) const;

 protected:
  typedef QVector< QVector<QVector3D> > Geometry;
  typedef QVector< QVector<QVector2D> > TextureCoords;

  virtual Geometry createGeometry(const QVector3D& size);
  virtual TextureCoords createTextureCoords(const Geometry& geometry);
  virtual TextureCoords createTextureCoordsForBlock(QVector<QVector3D> front, QVector<QVector3D> back);
  virtual Geometry moveToOrigin(const QVector3D& size, const Geometry& geometry);
  virtual void addGeometry(const Geometry& geometry, const TextureCoords& texture_coords);

  void appendVertex(const QVector3D &vertex,
                    const QVector3D &normal,
                    const QVector2D &tex_coord);

  void addQuad(const QVector3D &a, const QVector3D &b,
               const QVector3D &c, const QVector3D &d,
               const QVector<QVector2D> &tex);

  /**
    * Translates faces from \p from_orientation into the default orientation.
    * For example, if \p local_face is kFrontFace, and \p from_orientation is "Facing east" (which is a 90 degree
    * counter-clockwise rotation from the default orientation), then the return value will be kRightFace, because when
    * the right face is rotated 90 degrees counterclockwise, it occupies the same position that the front face would
    * occupy in the default orientation.
    * @param local_face The face to translate into the default orientation.
    * @param from_orientation The orientation to translate from.
    * @note This method is used when determining block adjacency information.  The render delegate does not account for
    * orientation when determining adjacency, so we have to map back to the default orientation before we consult it.
    */
  virtual Face mapToDefaultOrientation(Face local_face, const BlockOrientation* from_orientation) const;

 private:
  QVector3D size_;
  QVector<QVector3D> vertices_;
  QVector<QVector3D> normals_;
  QVector<QVector2D> tex_coords_;
};

#endif // RECTANGULAR_PRISM_RENDERABLE_H
