-- Paul Merrell's Model Synthesis Editor for 3D Studio Max

-- Return the name of a piece for any given number.  For example, 12 returns "0012piece"

fn pieceName x =
(
	((x / 1000) as string) + (((mod (x/100) 10) as integer) as string) + (((mod (x/10) 10) as integer) as string) + (((mod x 10) as integer) as string) + "piece";
)

fn round x = if x > 0 then ((x + 0.5) as integer) else ((x -  0.5) as integer)
	
rollout editor "Model Synthesis Editor" width: 300 height: 160
(
	local synthesis = #(),           -- A list of the model objects
		  filename = "Untitled",     -- The current file name
		  maxfile,					 -- The 3D Studio file that 
		  xsize = 0, 				 -- The size of the model
		  ysize = 0, 
		  zsize = 0,		 
		  dir = sysInfo.currentDir,	 -- Directory where the model synthesis files are
		  pieceNum = 0,              -- The number of model pieces  
		  model = #(#(#())),		 -- 3D array of model pieces
		  objects = #(#()); 		 -- Objects in the model pieces

	button generate "Open Model"         width: 100 enabled: true across: 2	offset: [-22,0]
	button latest   "Open Most Recent"	 width: 160 enabled: true offset: [-10,0]
	button new      "New Model" 	 	 width: 100 enabled: true offset: [0,0] across: 3
	button save		"Save Model"		 width: 100 enabled: false offset: [28,0]
	button saveAs   "As"        		 width:  50 enabled: false offset: [23,0]

	group "Current Model File"
	(
		label fileLabel  "Untitled"  align: #left offset: [20,0];
		label pieceLabel "Pieces: 0" align: #left offset: [20,10];
		label sizeLabel "Size: 0 x 0 x 0" align: #left offset: [175,-20];
		button piece	"Add Piece"		align: #left width: 80 offset:[0,10] enabled: true across: 3
		button origin   "Origin Move" width: 80 offset:[-10,10] enabled: false
		button create	"Create Model"  width: 100 offset:[0,10] enabled: false
	)	

	-- Clears out the models that have been generated.

	fn clear =
	(
		delete synthesis;
		synthesis = #();
	)
	
	on new pressed do
	(
		clear();
		objects = #(#());
		pieceNum = 0;
		fileLabel.caption  = "Untitled";
		pieceLabel.caption = "Pieces: 0";
		sizeLabel.caption  =  "Size: 0 x 0 x 0";
	)
	
	on origin pressed do
	(				
		for i = 1 to pieceNum do
		(
			obj = objects[i];
			realObj = #();
	
			realObj[1] = execute obj[1];
			minpos = realObj[1].min;
			maxpos = realObj[1].max;			
			
			for count = 2 to obj.count do
			(
				realObj[count] = execute obj[count];
				for j = 1 to 3 do
				(

					objMin = realObj[count].min[j];
					objMax = realObj[count].max[j];
					if objMin < minpos[j] then
						minpos[j] = objMin;
					if objMax > maxpos[j] then
						maxpos[j] = objMax;
				)
			)
							
			--  Bring the model pieces to the origin
			pos = (minpos + maxpos) / 2.0 / 10.0;
			x = (round pos[1]) + 1;
			y = (round pos[2]) + 1;
			z = (round pos[3]) + 1;
			move realObj [-x*10,-y*10,-z*10];
		)
	)
	
	fn open =
	(
		infile = openFile filename;
		
		if infile == undefined then
			messageBox "Model file does not exist."
		else
		(
		
		-- Display which file is currently open.
		
		fileLabel.caption = filename;			
		
		
		-- Ignore the first three lines.
		
		word = readline infile;					
		word = readline infile;
		word = readline infile;
		word = readline infile;
		
		-- Read in the extents of the object.
		
		word=word as stringstream
		xsize=readDelimitedString word " ";
		xsize=xsize as integer;
		ysize=readDelimitedString word " ";
		ysize=ysize as integer;
		zsize=readDelimitedString word " ";
		zsize=zsize as integer;

		
		-- Create a 3D array.
		
		model = #(#(#()));
		for x = 1 to xsize do
		(
			model[x] = #(#());
			for y = 1 to ysize do
			(
				model[x][y] = #();
			)
		)
		
		-- Read in the 3D array of integers.
		
		readline infile;
		types = 0;
		for z = 1 to zsize do
		(
		
			for x = 1 to xsize do
			(
				word = readline infile;
				word = word as stringstream;
				for y = 1 to ysize do
				(
					num  = readDelimitedString word " ";
					while (num == "") do
						num  = readDelimitedString word " ";	-- This deals with some white space issues.
					num  = num as integer;
					model[x][y][z] = num;
					if num > types then
					  types = num;
				)
			)
			readline infile;
		)
		
		---- Print out the 3D array
		--for z = 1 to zsize do
		--(
		--	for x = 1 to xsize do
		--	(
		--		temp = "";
		--		for y = 1 to ysize do
		--		(
		--			temp = temp + " " + (model[x][y][z] as string);
		--		)
		--		print temp;
		--	)
		--	print "";
		--)
			
		failed = true;
		ignoreQuery = false;
		objects = #(#())			-- Stores all the objects that get used.
		
		while failed do
		(
			missing = false;		-- If some of the objects are missing from the file.
			failed = false;
			
			-- Ignore everything until the objects appear.
			
			while word != "<Objects>" do
				word = readline infile;		
			word = readline infile;

			
			--    The first line in objects stores the 3D Studio where the objects are located.  
			-- We don't know if the user currently has this file open or not.  
			-- We'll assume the user has this file open, unless we discover that some of the 
			-- objects in the file are missing in which case we will ask the file if he wants
			-- to open this file.
			
			maxfile = word;
			word = readline infile;
			i = 1;

			while not eof infile and not failed do
			(
				copies = 0;
				obj = #();
				objnames = #();
				
				--    Ignore the first line.  Normally the number of the object appears here, but 
				-- sometimes it doesn't.  Instead we use the value i.
				
				word = readline infile;			
				count = 1;
				done = false;				-- done is needed to deal with weird end of file issues.
				
				--     Record all objects that appear until the character # appears.  These are the 
				-- objects that will be copied and pasted.
				
				while (not word == "#") and not done do 
				(    
					obj[count] = execute(word as string);
					objnames[count] = word;

					
			 		count = count + 1;
					if eof infile then
					 	done = true;
					else
				    	word = readline infile;
				)

				if not done then
				(
					if not eof infile then
						word = readline infile; 
					objects[i] = objnames;			
		
					-- Go through the entire 3D array and try to find any entry where the value i appears.
					--   Copy and paste the objects into that location.
		
					x = 1;
					while (x <= xsize and not failed) do
					(
						y = 1;
						while (y <= ysize and not failed) do
						(
							z = 1;
							while (z <= zsize and not failed) do
							(
								-- Test if i is here.
								
								if model[x][y][z] == i then
								(
									moving = #();
									j = 1;
									while (j <= obj.count and not failed) do
									(
										-- Test if each object can be found in the 3D Studio file.  If an object is missing, the 
										--     user probably has the wrong 3D Studio file open and we ask the user if he wants 
										--     to open up the proper 3D Studio file.
									
										if obj[j] == undefined then
										(
											if not ignoreQuery and queryBox ("Some objects are missing from the current 3D Studio file.\n\nThey might be in the file " + maxfile + "\n\nDo you want to open this file?") then
											(
												if not loadMaxFile maxfile then
													if not loadMaxFile (dir + "\\" + maxfile) then			-- The maxfile may be written with or without the directory in front of it.  Try both cases.
														messageBox "3D Studio scene file was not found.";
												failed = true;
												close infile;
												infile = openFile filename;
											)
											ignoreQuery = true;		
											missing = true;							
										)
										else
										(
											-- Copy each object.  Store them all in synthesis, so we can delete them later if we want to.
											
											moving[j] = instance obj[j];
											synthesis[synthesis.count+1] = moving[j];
										)
										j = j + 1;
									)
									
									-- Move the objects to the correct location.  Each cell of the 3D grid is spaced 10 units apart.
									
									move moving [x*10,y*10,z*10];
									
									-- Group the objects together.  This part isn't entirely necessary.
									
									--if moving.count > 0 then
									--(
									--	grouped = group moving;
									--	grouped.name = (pieceName i) + (copies as string);
									--	copies = copies + 1;
									--)
								)
								z = z + 1;
							)
							y = y + 1;
						)
						x = x + 1;
					)	
					i = i + 1;    -- Increment i.
				)
			)
		)
		
		-- Display the size of the model and the number of model pieces
		
		sizeLabel.caption = "Size: " + (xsize as string) + " x " + (ysize as string) + " x " + (zsize as string);
		pieceNum = i-1;
		pieceLabel.caption = "Pieces: " + (pieceNum as string);
		
		
		-- Display if some of the objects are still missing.
		
		if missing then
			messageBox "Some of the objects were not copied because they could not be found in the scene.";
		
		-- Close file and update the GUI.
		
		close infile;
		save.enabled = true;
		saveAs.enabled = true;
		create.enabled = true;
		origin.enabled = true;
		print ("Model Successfully Generated from " + filename);
		)
	)
	
	on latest pressed do
	(
		infile = openFile "latest.txt";
		word = readline infile;
		filename = readline infile;
		fileLabel.caption = filename;
		
		open();
	)
	
	on generate pressed do
	(
		filename = getOpenFileName	filename: (dir + "\\Example Models\\") types:"Text Files(*.txt)|*.txt|All Files|*.*";
		if filename != undefined then
			open();
	)
	
	on piece pressed do
	(
		selected = getCurrentSelection();
		if selected.count == 0 then
			messageBox "No objects are currently selected."
		else
		(
			--x = 1;
			--while (x < pieceNum) and ((execute ("$" + (pieceName x))) != undefined) do
			--	x = x + 1;
			x = pieceNum+1;
			
			objNames = #();	
			for i = 1 to selected.count do
			(
				objNames[i] = "$" + selected[i].name;
			)
			objects[x] = objNames;
			
			if x > pieceNum then
			(
				pieceNum = x;
				pieceLabel.caption = "Pieces: " + (pieceNum as string);
			)
				
			grouped = group $selection;
			grouped.name = pieceName x;
			select grouped;
			create.enabled = true;			
			save.enabled   = true;
			saveAs.enabled = true;
			origin.enabled = true; 
		)
	)
	
	on create pressed do
	(
		selected = getCurrentSelection();
		if selected.count == 0 then
			messageBox "No objects are currently selected."
		else
		(
			xmin = 1; xmax = 1; ymin = 1; ymax = 1; zmin = 1; zmax = 1;
			pieces = #();
			for i = 1 to selected.count do
			(
				sName = selected[i].name;
				if (sName.count >= 9 ) and (substring sName 5 5 == "piece") then
				(
					--pos = (selected[i].min + selected[i].max) / 2.0 / 10.0;
					
					original = (execute ("$" + (substring sName 1 9)));
					pos = (selected[i].center - original.center) / 10.0;
					
					x = round pos[1];
					y = round pos[2];
					z = round pos[3];
								
					pieces[pieces.count+1] = #((execute (substring sName 1 4)),x,y,z);

					if pieces.count == 1 then
					(
						xmin = x;
						ymin = y;
						zmin = z;
						xmax = x;
						ymax = y;
						zmax = z;
					)
					
					if x > xmax then
						xmax = x;					
					if y > ymax then
						ymax = y;					
					if z > zmax then
						zmax = z;
					if x < xmin then
						xmin = x;					
					if y < ymin then
						ymin = y;					
					if z < zmin then
						zmin = z;
				)
			)
			
			if xmax == undefined then
				messageBox "The selection does not contain any model pieces.";
			else
			(			
				xsize = xmax - xmin + 1;
				ysize = ymax - ymin + 1;
				zsize = zmax - zmin + 2;
				
				sizeLabel.caption = "Size: " + (xsize as string) + " x " + (ysize as string) + " x " + (zsize as string);
				
				if xsize > 200 or ysize > 200 or zsize > 100 then
					messageBox "This example model is exceptionally large."
				
				if xsize < 4 or ysize < 4 then
					messageBox "This example model is unusually small."
				
				model = #(#(#()));
				for x = 1 to xsize do
				(
					model[x] = #(#());
					for y = 1 to ysize do
					(
						model[x][y] = #();
						for z = 1 to zsize do
							model[x][y][z] = 0;
					)
				)
				
				types = 0;
				for i = 1 to pieces.count do
				(
					p = pieces[i];
					model[p[2]-xmin+1][p[3]-ymin+1][p[4]-zmin+1] = p[1];
					if p[1] > types then
						types = p[1];
				)
			)
		)
	)
	
	
	fn write =
	(	
		outfile = createFile filename;
		
		format "Example Model.  Do not insert or delete lines from this file.\n" to: outfile;
		format ("\n") to: outfile;
		format "x, y, and z extents" to: outfile;
		format ("\n" + (xsize as string) + " " + (ysize as string) + " " + (zsize as string) + "\n\n") to: outfile;
		
		for z = 1 to zsize do
		(
			for x = 1 to xsize do
			(
				for y = 1 to ysize do
				(
					num = model[x][y][z];
					if num < 10 then
						format " " to: outfile;
					format ((num as string) + " ")  to: outfile;
				)
				format "\n" to: outfile;
			)
			format "\n" to: outfile;
		)
		format "<Objects>\n" to:outfile;
		format (maxfile + "\n") to:outfile;
		
		for i = 1 to pieceNum do 
		(
			format ((i as string) + "\n") to: outfile;
			for j = 1 to objects[i].count do
			(
				format (objects[i][j] + "\n") to: outfile;
			)
			format "#\n" to: outfile;
		)
		close outfile;
	)
	
	fn writeAs = 
	(
		filename = getSaveFileName caption: "Save Example Model" filename: (dir + "\\Example Models\\") types: "Text Files(*.txt)|*.txt|All Files|*.*";
		if filename != undefined then
		(
			maxfile = getOpenFileName caption: "Current 3D Studio Max File" filename: (dir + "\\3D Studio Scenes\\") types: "3ds max(*.max)|*.max|All Files|*.*";
			if maxfile != undefined then
				write();
		)	
	)
	
	on save pressed do
	(
		if filename == "Untitled" then
			writeAs();
		else
			write();
	)
	
	on saveAs pressed do
	(
		writeAs();
	)
)


modelEditor = CreateDialog editor;