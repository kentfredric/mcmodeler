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

#include "renderable.h"

Texture Renderable::texture(int local_id) const {
  return textures_[local_id];
}

void Renderable::setTexture(int local_id, const Texture& texture) {
  if (local_id >= textures_.size()) {
    textures_.resize(local_id + 1);
  }
  textures_[local_id] = texture;
}

