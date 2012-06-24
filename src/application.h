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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

#include "block_manager.h"
#include "diagram.h"

class Application : public QApplication {
  Q_OBJECT
 public:
  explicit Application(int& argc, char** argv);
  virtual ~Application();

 private:
  QScopedPointer<Diagram> diagram_;
  QScopedPointer<BlockManager> block_mgr_;
};

#endif // APPLICATION_H