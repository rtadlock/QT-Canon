/*
  main.cpp
 
  Copyright (C) 2009 Robert Tadlock and Jeremiah LaRocco

  This file is part of QtCanon

  QtCanon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  QtCanon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with QtCanon.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <iostream>

#include "tabdialog.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    TabDialog tabdialog;

    return tabdialog.exec();
}
