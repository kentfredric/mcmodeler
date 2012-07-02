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

#include "block_prototype.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QFile>
#include <QMap>
#include <QPoint>
#include <QString>
#include <QVector>

#include <QJson/Parser>

#include "bed_renderable.h"
#include "block_oracle.h"
#include "block_position.h"
#include "door_renderable.h"
#include "ladder_renderable.h"
#include "overlapping_faces_renderable.h"
#include "rectangular_prism_renderable.h"
#include "renderable.h"
#include "stairs_renderable.h"
#include "texture.h"
#include "texture_pack.h"

QMap<blocktype_t, BlockProperties>* BlockPrototype::s_type_mapping = NULL;

// Static.
void BlockPrototype::setupBlockProperties() {
#ifdef Q_OS_MACX
  // Use CoreFoundation to get the bundle path so we're not working-directory dependent.
  CFBundleRef bundle = CFBundleGetMainBundle();
  CFURLRef blocks_url = CFBundleCopyResourceURL(bundle, CFSTR("blocks"), CFSTR("json"), NULL);
  CFStringRef blocks_path_cf = CFURLCopyPath(blocks_url);
  CFRelease(blocks_url);
  QString blocks_path;
  CFIndex len = CFStringGetLength(blocks_path_cf);
  blocks_path.resize(len);
  CFStringGetCharacters(blocks_path_cf, CFRangeMake(0, len), reinterpret_cast<UniChar*>(blocks_path.data()));
  CFRelease(blocks_path_cf);
  QFile f(blocks_path);
#else
  QFile f("blocks.json");
#endif
  if (!f.exists()) {
    QMessageBox msg;
    msg.setIcon(QMessageBox::Critical);
    msg.setWindowTitle(qApp->applicationName());
#ifdef Q_OS_MACX
    msg.setText("The blocks.json file could not be found in the Resources folder inside the application bundle.");
#else
    msg.setText("The blocks.json file could not be found in the application directory.");
#endif
    msg.setInformativeText(
        QString("Without this file, %1 cannot continue. Please replace it or reinstall and try again.")
        .arg(qApp->applicationName()));
    msg.exec();
    return;
  }

  QJson::Parser p;
  bool success = false;
  QVariant root = p.parse(&f, &success);
  if (!success) {
    QMessageBox msg;
    msg.setIcon(QMessageBox::Critical);
    msg.setWindowTitle(qApp->applicationName());
    msg.setText("The blocks.json file contained invalid JSON text and could not be read.");
    msg.setInformativeText(QString("%1 on line %2.").arg(p.errorString()).arg(p.errorLine()));
    msg.exec();
    return;
  }

  s_type_mapping = new QMap<blocktype_t, BlockProperties>();
  QVariantList blocks = root.toList();
  int i = 0;
  foreach (QVariant block_variant, blocks) {
    QVariantMap block = block_variant.toMap();
    s_type_mapping->insert(i, BlockProperties(block));
    ++i;
  }
}

// static
QString BlockPrototype::nameOfType(blocktype_t type) {
  if (s_type_mapping && s_type_mapping->contains(type)) {
    return s_type_mapping->value(type).name();
  } else {
    return QString();
  }
}

// static
int BlockPrototype::blockCount() {
  if (!s_type_mapping) {
    return 0;
  }
  return s_type_mapping->size();
}

const BlockProperties& BlockPrototype::properties() const {
  return properties_;
}

BlockPrototype::BlockPrototype(blocktype_t type, TexturePack* texture_pack, BlockOracle* oracle, QGLWidget* widget)
    : type_(type), oracle_(oracle) {
  if (!s_type_mapping) {
    qWarning() << "You forgot to call setupBlockProperties!";
    s_type_mapping = new QMap<blocktype_t, BlockProperties>();
  }

  properties_ = s_type_mapping->value(type_);
  switch (properties_.geometry()) {
    case kBlockGeometryCube:
      renderable_.reset(new RectangularPrismRenderable(QVector3D(1.0f, 1.0f, 1.0f)));
      break;
    case kBlockGeometrySlab:
      renderable_.reset(new RectangularPrismRenderable(QVector3D(1.0f, 0.5f, 1.0f)));
      break;
    case kBlockGeometryChest:
      renderable_.reset(new RectangularPrismRenderable(QVector3D(0.9f, 0.9f, 0.9f),
                                                       RectangularPrismRenderable::kTextureScale));
      break;
    case kBlockGeometryPressurePlate:
      renderable_.reset(new RectangularPrismRenderable(QVector3D(0.8f, 0.05f, 0.8f)));
      break;
    case kBlockGeometryStairs:
      renderable_.reset(new StairsRenderable(QVector3D(1.0f, 1.0f, 1.0f)));
      break;
    case kBlockGeometryCactus:
      renderable_.reset(new OverlappingFacesRenderable(QVector3D(1.0f, 1.0f, 1.0f),
                                                       QVector3D(0.0625f, 0.0f, 0.0625f)));
      break;
    case kBlockGeometryBed:
      renderable_.reset(new BedRenderable());
      break;
    case kBlockGeometryDoor:
      renderable_.reset(new DoorRenderable());
      break;
    case kBlockGeometryLadder:
      renderable_.reset(new LadderRenderable());
      break;
    default:
      qWarning() << "No renderable could be found for block" << properties_.name();
      renderable_.reset(new RectangularPrismRenderable(QVector3D(1.0f, 1.0f, 1.0f)));
      break;
  }
  renderable_->initialize();
  renderable_->setRenderDelegate(this);

  QPixmap terrain_png = texture_pack->tileSheetNamed("terrain.png");

  QVector<QPoint> tiles = properties_ .tileOffsets();
  for (int i = 0; i < tiles.size(); ++i) {
    if (properties_.isBiomeGrass() && i == 4) {
      Texture t(widget, terrain_png, tiles[i].x(), tiles[i].y(), 16, 16,
                QColor(0x60, 0xC6, 0x49, 0xFF), QPainter::CompositionMode_Multiply);
      renderable_->setTexture(static_cast<Face>(i), t);
    } else if (properties_.isBiomeTree()) {
      Texture t(widget, terrain_png, tiles[i].x(), tiles[i].y(), 16, 16,
                QColor(0x58, 0x6C, 0x2F, 0xFF), QPainter::CompositionMode_Multiply);
      renderable_->setTexture(static_cast<Face>(i), t);
    } else {
      Texture t(widget, terrain_png, tiles[i].x(), tiles[i].y(), 16, 16);
      renderable_->setTexture(static_cast<Face>(i), t);
    }
  }
  QPoint sprite_offset = properties_.spriteOffset();
  if (!properties_.isValid()) {
    sprite_texture_ = Texture(widget, ":/null_sprite.png", 0, 0, 16, 16);
  } else if (properties_.isBiomeGrass()) {
    sprite_texture_ = Texture(widget, terrain_png, sprite_offset.x(), sprite_offset.y(), 16, 16,
                              QColor(0x60, 0xC6, 0x49, 0xFF), QPainter::CompositionMode_Multiply);
  } else if (properties_.isBiomeTree()) {
    sprite_texture_ = Texture(widget, terrain_png, sprite_offset.x(), sprite_offset.y(), 16, 16,
                              QColor(0x58, 0x6C, 0x2F, 0xFF), QPainter::CompositionMode_Multiply);
  } else {
    sprite_texture_ = Texture(widget, terrain_png, sprite_offset.x(), sprite_offset.y(), 16, 16);
  }
  sprite_engine_.reset(new SpriteEngine());
}

QPixmap BlockPrototype::sprite(BlockOrientation* orientation) const {
  return sprite_engine_->createSprite(sprite_texture_, properties_, orientation);
}

BlockOrientation* BlockPrototype::defaultOrientation() const {
  if (properties().validOrientations().empty()) {
    return BlockOrientation::noOrientation();
  } else {
    return properties().validOrientations().at(0);
  }
}

QVector<BlockOrientation*> BlockPrototype::orientations() const {
  QVector<BlockOrientation*> orientations = properties().validOrientations();
  if (orientations.empty()) {
    orientations.append(BlockOrientation::noOrientation());
  }
  return orientations;
}

void BlockPrototype::renderInstance(const BlockInstance& instance) const {
  renderable_->renderAt(instance.position().centerVector(), instance.orientation());
}

// TODO(phoenix): This doesn't look like it belongs here.  Shouldn't the Renderable be responsible for this?
bool BlockPrototype::shouldRenderFace(const Renderable* renderable, Face face, const QVector3D& location) const {
  Q_UNUSED(renderable);

  BlockGeometry geometry = properties().geometry();
  if (geometry != kBlockGeometryCube && geometry != kBlockGeometrySlab) {
    return true;
  }

  BlockPrototype* other = neighboringBlockForFace(face, location);
  return (other->type() ==  kBlockTypeAir ||
          other->properties().geometry() != kBlockGeometryCube ||
          (other->properties().isTransparent() && other->type() != type()));
}

BlockPrototype* BlockPrototype::neighboringBlockForFace(Face face, const QVector3D& location) const {
  static QVector3D front = QVector3D(0, 0, 1);
  static QVector3D back = QVector3D(0, 0, -1);
  static QVector3D left = QVector3D(-1, 0, 0);
  static QVector3D right = QVector3D(1, 0, 0);
  static QVector3D top = QVector3D(0, 1, 0);
  static QVector3D bottom = QVector3D(0, -1, 0);

  if (!oracle_) {
    return NULL;
  }

  if (face == kFrontFace) {
    return oracle_->blockAt(BlockPosition(location + front)).prototype();
  }
  if (face == kBackFace) {
    return oracle_->blockAt(BlockPosition(location + back)).prototype();
  }
  if (face == kLeftFace) {
    return oracle_->blockAt(BlockPosition(location + left)).prototype();
  }
  if (face == kRightFace) {
    return oracle_->blockAt(BlockPosition(location + right)).prototype();
  }
  if (face == kTopFace) {
    return oracle_->blockAt(BlockPosition(location + top)).prototype();
  }
  if (face == kBottomFace) {
    return oracle_->blockAt(BlockPosition(location + bottom)).prototype();
  }
  return NULL;
}