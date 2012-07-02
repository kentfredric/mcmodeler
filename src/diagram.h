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

#ifndef DIAGRAM_H
#define DIAGRAM_H

#include <QMap>
#include <QObject>
#include <QVector>
#include <QVector3D>

#include "block_instance.h"
#include "block_oracle.h"
#include "block_position.h"
#include "block_prototype.h"
#include "block_type.h"

class BlockManager;
class BlockOrientation;
class BlockTransaction;

/**
  * Represents a diagram containing block data for the world.
  *
  * The diagram is the model class responsible for keeping track of what blocks exist at what locations, and for
  * serializing and deserializing that data upon request.  It provides some convenience methods for performing batch
  * operations on blocks, such as flood filling and drawing lines.
  *
  * All operations on the Diagram take place within the scope of a BlockTransaction.  You can either call one of the
  * convenience methods, which create and commit a BlockTransaction for you, or you can create a BlockTransaction
  * that describes the change you'd like to make and then call commit() on the Diagram yourself.  Whenever a change is
  * made to the diagram, the diagramChanged() signal is emitted with the transaction that was performed.
  *
  * Diagram treats the world as horizontal slices, each corresponding to a level in the LevelWidget.  You can get a
  * map of a given level by calling the level() method.  You can also look up the block at a particular 3D location by
  * calling the blockAt() method.
  */
class Diagram : public QObject, public BlockOracle {
  Q_OBJECT
 public:
  Diagram(QObject* parent = NULL);

  /**
    * Sets the block manager for this diagram.  The block manager is used to get the prototypes for blocks in the map.
    * Diagram cannot function without a block manager, so make sure to set one before you call any other methods.
    */
  void setBlockManager(BlockManager* block_mgr) {
    block_mgr_ = block_mgr;
  }

  /**
    * Returns the BlockInstance for the block at \p position.  If there is no block at that location, the return value
    * will have a prototype whose type is kBlockTypeAir, a position of \p position, and no orientation.
    * @note The instance returned from this method will _always_ have a position of \p position.
    */
  virtual BlockInstance blockAt(const BlockPosition& position);

  /**
    * Tells all blocks in the diagram to render themselves.
    * @todo This probably does not belong in the Diagram class, but it's unclear where it should go instead.
    */
  void render();

  /**
    * Saves all blocks in the diagram out to \p stream.  The save format is versioned, so incompatible changes should
    * probably occasion a change to the version number.
    */
  void save(QDataStream* stream);

  /**
    * Populates the diagram with blocks deserialized from \p stream.
    */
  void load(QDataStream* stream);

  /**
    * Creates and commits a BlockTransaction which replaces the block at \p pos with \block.  The position of \p block
    * must be the same as \p pos.  This is enforced.
    */
  void setBlock(const BlockPosition& pos, const BlockInstance& block);

  /**
    * Creates and commits a BlockTransaction which clears the block at \p pos.
    */
  void clearBlock(const BlockPosition& pos);

  /**
    * Creates and commits a BlockTransaction which performs a two-dimensional flood fill of blocks starting at \p
    * start_pos.  Any blocks contiguous with \p start_pos which have the same y coordinate and type as the block found
    * there will be changed to \p type and assigned \p orientation.
    * @warning We currently use a fairly naive flood fill algorithm.  In the presence of an infinite canvas, this can
    * present some problems.  The algorithm will eventually stop filling after a large number of iterations, but it can
    * fill a vast number of blocks.
    */
  void fillBlocks(const BlockPosition& start_pos, const blocktype_t& type, BlockOrientation* orientation);

  /**
    * Creates and commits a BlockTransaction which draws a line of blocks with type \p type and orientation \p
    * orientation from \p start_pos to \p end_pos.  The line is currently always one block thick.
    * @bug This algorithm has a bug which can occasionally cause it to hang.  A sanity check exists to prevent it from
    * hanging infinitely, but the behavior is still mildly unpleasant.
    */
  void drawLine(const BlockPosition& start_pos, const BlockPosition& end_pos,
                const blocktype_t& type, BlockOrientation* orientation);

  /**
    * Creates and commits a BlockTransaction which copies all blocks on \p source_level to \p dest_level.
    * @note A "level" is currently defined to be the set of blocks sharing a particular _y_ coordinate.
    */
  void copyLevel(int source_level, int dest_level);

  /**
    * Applies \p transaction to the diagram.  You can call this to perform custom transactions not covered by the
    * convenience methods above.
    */
  void commit(const BlockTransaction& transaction);

  void commitEphemeral(const BlockTransaction& transaction);

  /**
    * Returns the number of blocks in the diagram.
    */
  int blockCount() const;

  /**
    * Returns a dictionary of counts for every block type that appears at least once in the diagram.
    */
  QMap<blocktype_t, int> blockCounts() const;

  /**
    * Returns all the blocks on the level \p level_index.  Each BlockInstance in the returned dictionary will have a
    * _y_ coordinate of \p level_index, and will be keyed on its own position for easy lookup.
    */
  QHash<BlockPosition, BlockInstance> level(int level_index);

 signals:
  /**
    * Emitted when the diagram changes.
    * @param transaction The BlockTransaction that caused the change.
    */
  void diagramChanged(const BlockTransaction& transaction);

  void ephemeralBlocksChanged(const BlockTransaction& transaction);

 private:
  /**
    * Returns the block manager, or NULL if it's not set.  This method exists mainly to fire an assert if it is called
    * while the BlockManager is NULL.  For that reason, you should always call this instead of accessing block_mgr_
    * directly.
    */
  BlockManager* blockManager() const;

  /**
    * Directly adds a block to the diagram, replacing whatever block was there.  This should only be called from
    * commit() unless you know what you're doing, since it will neither fire diagramChanged() nor create a
    * BlockTransaction for the change.
    */
  void addBlockInternal(const BlockInstance& block);

  void addEphemeralBlockInternal(const BlockInstance& block);

  /**
    * Directly removes a block from the diagram.  This should only be called from commit() unless you know what you're
    * doing, since it will neither fire diagramChanged() nor create a BlockTransaction for the change.
    */
  void removeBlockInternal(const BlockPosition& position);

  /**
    * Recursion helper function for fillBlocks.
    * @param pos The current position we are filling.
    * @param source_type The type of the block we should replace.
    * @param dest_type The type of block we should replace it with.
    * @param dest_orientation The orientation we should set newly created blocks to.
    * @param start_pos The point at which the initial fillBlocks began.
    * @param depth Recursive stack depth.  Used for sanity checking to avoid stack overflow.
    * @param filled_blocks The set of all blocks we have filled so far.  This is shared among all the recursive helpers.
    * @param transaction The transaction we are building which, when committed, will perform the flood fill.
    */
  void fillBlocksRecurse(const BlockPosition& pos,
                         const blocktype_t& source_type,
                         const blocktype_t& dest_type,
                         BlockOrientation* dest_orientation,
                         const BlockPosition& start_pos,
                         int depth,
                         QSet<BlockPosition>* filled_blocks,
                         BlockTransaction* transaction);

  /**
    * A map of all blocks in the diagram, regardless of level.  Used mainly for rendering.
    */
  QHash<BlockPosition, BlockInstance> block_map_;

  /**
    * A map from level to the blocks at that level.  This allows us to display a given level without having to perform
    * an incremental search for blocks with the right y-coordinate.
    */
  QHash< int, QHash<BlockPosition, BlockInstance> > block_list_;

  /**
    * A map of the ephemeral blocks in the diagram.
    */
  QHash<BlockPosition, BlockInstance> ephemeral_blocks_;

  /**
    * The block manager set using setBlockManager.  Can technically be NULL, but shouldn't be by the time any other
    * methods are called.  Don't access this directly -- use blockManager() instead.
    */
  BlockManager* block_mgr_;
};

#endif // DIAGRAM_H