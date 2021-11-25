## Written by Jason Sewall
## Modified by Paul Merrell
## Updated for Blender 3.0 by Daklinus
import bpy
import os
import math
import mathutils
import re
import array
import itertools
from bpy.props import StringProperty, BoolProperty
from bpy_extras.io_utils import ImportHelper
from bpy.types import Operator
from math import radians
from mathutils import Matrix

extents_re = re.compile(r"^x, y, and z extents")
objects_start_re = re.compile(r"<Objects>")
start_name_re    = re.compile(r"^(\d+)?\s*(?:\$([^\s]*))?")
name_re    = re.compile(r"^\s*\$([^\s]*)")
end_re     = re.compile("^#\s*")

def make_scndict(scn):
    scndict = {}
    for i in scn.objects:
        scndict[i.name] = i
    return scndict

def read_grid(infile):
    li = infile.readline()
    dim = [int(x) for x in li.rstrip().split(' ')]
    grid = array.array('L', itertools.repeat(0, dim[0]*dim[1]*dim[2]))
    for z in range(0, dim[2]):
        li = infile.readline()
        for x in range(0, dim[0]):
            li = [int(it) for it in infile.readline().rstrip().split()]
            for y in range(0, dim[1]):
                grid[z*dim[1]*dim[0] + x*dim[1] + y] = li[y]
    return (dim, grid)

def read_object(infile, li):
    res = start_name_re.match(li)
    grps = res.groups()
    if(grps[0] != None):
        num = int(grps[0])
    else:
        num = None
    entry = []
##  Apparently, we don't want to use any objects the appear on
##  the same line as the number, so don't use any other groups
##    if(grps[1] != None):
##        entry.append(grps[1])
    li = infile.readline()
    res = end_re.match(li)
    while res == None:
        res = name_re.match(li)
        assert(res != None)
        entry.append(res.group(1))
        li = infile.readline()
        res = end_re.match(li)
    return (num, entry)

def read_objects(infile):
    scenefile = infile.readline()
    objs = []
    li = infile.readline()
    while li != "":
        if(li.rstrip() != ""):
            num, entry = read_object(infile, li)
            print(num, entry)
            objs.append(entry)
        li = infile.readline()
    return (objs,scenefile)

def read_synth(infile):
    ## Look for start of grid (skip until extent warning)
    li = infile.readline()
    res = extents_re.match(li)
    while li != "" and res == None:
        li = infile.readline()
        res = extents_re.match(li)
    (dim, grid) = read_grid(infile)
    print ("Read %d x %d x %d grid" % (dim[0], dim[1], dim[2]))
    ## Look for start of objects (skip until object tag)
    li = infile.readline()
    res = objects_start_re.match(li)
    while li != "" and res == None:
        li = infile.readline()
        res = objects_start_re.match(li)
    (objs, scenefile) = read_objects(infile)
    print("Read %d object groups" % (len(objs),))
    return (dim, grid, objs, scenefile)

def fuse_objects(scndict, objs):

    res = [None]
    found = 0
    missing = 0
    for olist in objs:
        oolist = []
        for obj in olist:
            if obj in scndict:
                oolist.append(scndict[obj])
                found = 1
            else:
                missing = 1
        res.append(oolist)
    
    return (res, found, missing)

def object_array(scn, dim, grid, objs, units):
    for z in range(0, dim[2]):
        for x in range(0, dim[0]):
            for y in range(0, dim[1]):
                current_objs = objs[grid[z*dim[1]*dim[0] + x*dim[1] + y]]
                if current_objs == None:
                    continue
                
                mv = mathutils.Matrix.Translation((x*units,y*units,z*units))
                
                for o in current_objs:
                    new_o = bpy.data.objects.new(o.name, o.data)
                    scn.collection.objects.link(new_o) 
                    new_o.matrix_world = mv
                    bpy.context.view_layer.update()
        print ("Done with plane ", z)

def read_file(filename):

    infile = open(filename + ".txt")

    scn = bpy.context.window.scene
    scndict = make_scndict(bpy.context.window.scene)
    (dim, grid, objs, scenefile) = read_synth(infile)
    (objs2, found, missing) = fuse_objects(scndict, objs)
    scenefile = scenefile.rstrip();
    
    if missing == 1:
        print("Blender Scenes/" + scenefile + " is not open.")
        fullpath = Blender.sys.expandpath("//" + scenefile)
        if Blender.sys.exists(fullpath) == 1:
            print("Opening " + scenefile + "...")
            Blender.Load(fullpath)
            print("a")
            scn = bpy.data.scenes.active
            print("b")
            scndict = make_scndict(bpy.data.scenes.active)
            print("c")
            (objs2, found, missing) = fuse_objects(scndict, objs)
            print("d")
            print(found)
    
    if found == 1:
        unit = 10
        print("Building object array")
        object_array(scn, dim, grid, objs2, unit)
        print("Done")
    else:
        print("The objects required in the example model could not be found in the current scene.Open the file Blender Scenes/" + scenefile + " and run the script again.")
    
    if found == 1 and missing == 1:
        print("Some of the objects are missing.  Check if the file Blender Scenes/" + scenefile + " is open.")
            
    infile.close()

class OT_TestOpenFilebrowser(Operator, ImportHelper):

    bl_idname = "test.open_filebrowser"
    bl_label = "Select Model"
    
    filter_glob: StringProperty(
        default='*.txt',
        options={'HIDDEN'}
    )
    
    
    def execute(self,context):
        filename, extension = os.path.splitext(self.filepath)
        read_file(filename)
        return {'FINISHED'}
        
def register():
    bpy.utils.register_class(OT_TestOpenFilebrowser)

def unregister():
    bpy.utils.unregister_class(OT_TestOpenFilebrowser)

if __name__ == '__main__':
    register()
    bpy.ops.test.open_filebrowser('INVOKE_DEFAULT')