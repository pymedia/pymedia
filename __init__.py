##    Copyright (C) 2002-2003  Dmitry Borisov
##
##    This library is free software; you can redistribute it and/or
##    modify it under the terms of the GNU Library General Public
##    License as published by the Free Software Foundation; either
##    version 2 of the License, or (at your option) any later version.
##
##    This library is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##    Library General Public License for more details.
##
##    You should have received a copy of the GNU Library General Public
##    License along with this library; if not, write to the Free
##    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##

"""PyMedia is a set of Python modules designed for writing media centers.
It uses pygame as a lower level for representing the visual information.
It provides with the complete framework for building menu driven inetrfaces. 
Some of the classes allows you to define your own behaviors and configuration files.
"""

__all__= [ 'pypower', 'menu' ]
import pypower, menu, parser

__version__= "1.2"