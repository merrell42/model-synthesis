## Written by Jason Sewall
## Modified by Paul Merrell

import Blender
import bpy
import math
import re
import array
import itertools

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
    for z in xrange(0, dim[2]):
        li = infile.readline()
        for x in xrange(0, dim[0]):
            li = [int(it) for it in infile.readline().rstrip().split()]
            for y in xrange(0, dim[1]):
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
            print num, entry
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
    print "Read %d x %d x %d grid" % (dim[0], dim[1], dim[2])
    ## Look for start of objects (skip until object tag)
    li = infile.readline()
    res = objects_start_re.match(li)
    while li != "" and res == None:
        li = infile.readline()
        res = objects_start_re.match(li)
    (objs, scenefile) = read_objects(infile)
    print "Read %d object groups" % (len(objs),)
    return (dim, grid, objs, scenefile)

def fuse_objects(scndict, objs):
    res = [None]
    found = 0
    missing = 0
    for olist in objs:
        oolist = []
        for obj in olist:
            if obj in scndict:
            	oolist.append(scndict[obj].getData(mesh=1))
            	found = 1
            else:
            	missing = 1
        res.append(oolist)
	
    return (res, found, missing)

def object_array(scn, dim, grid, objs, units):
    for z in xrange(0, dim[2]):
        for x in xrange(0, dim[0]):
            for y in xrange(0, dim[1]):
                current_objs = objs[grid[z*dim[1]*dim[0] + x*dim[1] + y]]
                if current_objs == None:
                    continue
                mat = Blender.Mathutils.Matrix([1.0, 0.0, 0.0, 0.0],
                                               [0.0, 1.0, 0.0, 0.0],
                                               [0.0, 0.0, 1.0, 0.0],
                                               [x*units, y*units, z*units, 1.0])

                for o in current_objs:
                    new_o = scn.objects.new(o)
                    new_o.setMatrix(mat)
        print "Done with plane ", z

def read_file(filename):
    infile = open(filename)

    scn = bpy.data.scenes.active
    scndict = make_scndict(bpy.data.scenes.active)
    (dim, grid, objs, scenefile) = read_synth(infile)
    (objs2, found, missing) = fuse_objects(scndict, objs)
    scenefile = scenefile.rstrip();
	
    if missing == 1:
		print "Blender Scenes/" + scenefile + " is not open. "
		fullpath = Blender.sys.expandpath("//" + scenefile)
		if Blender.sys.exists(fullpath) == 1:
			print "Opening " + scenefile + "..."
			Blender.Load(fullpath)
			print "a"
			scn = bpy.data.scenes.active
			print "b"
			scndict = make_scndict(bpy.data.scenes.active)
			print "c"
			(objs2, found, missing) = fuse_objects(scndict, objs)
			print "d"
			print found
	
    if found == 1:
		unit = 10
		print "Building object array"
		object_array(scn, dim, grid, objs2, unit)
		print "Done"
    else:
		print "The objects required in the example model could not be found in the current scene.  Open the file Blender Scenes/" + scenefile + " and run the script again."
	
    if found == 1 and missing == 1:
		print "Some of the objects are missing.  Check if the file Blender Scenes/" + scenefile + " is open."
			
    infile.close()

if __name__ == '__main__':
    Blender.Window.FileSelector(read_file,"Select Model")