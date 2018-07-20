#!/usr/bin/perl

#
#  GDML fragment generator for DUNE dual-phase 10kt detector geometry 
#  adapted from single phase cryostat geometry script from Tyler Dalion
#  by Vyacheslav Galymov < vgalymov@ipnl.in2p3.fr >
#
#  !!!NOTE!!!: the readout is on a positive X -- a fix for electric field
#              direction  problem in larsoft

#################################################################################
#
# contact tylerdalion@gmail.com for any GDML/generate questions
# I would love to help!

# Each subroutine generates a fragment GDML file, and the last subroutine
# creates an XML file that make_gdml.pl will use to appropriately arrange
# the fragment GDML files to create the final desired DUNE GDML file, 
# to be named by make_gdml output command

# If you are playing with different geometries, you can use the
# suffix command to help organize your work.
#
##################################################################################


#use warnings;
use gdmlMaterials;
use Math::Trig;
use Getopt::Long;
use Math::BigFloat;
Math::BigFloat->precision(-16);

GetOptions( "help|h" => \$help,
	    "suffix|s:s" => \$suffix,
	    "output|o:s" => \$output,
	    "wires|w:s" => \$wires,  
            "workspace|k:s" => \$wkspc); 

my $pmt_switch="on";
my $FieldCage_switch="on";
my $GroundGrid_switch="on";
my $Cathode_switch="on";
my $ExtractionGrid_switch="on";
my $LEMs_switch="on";


if ( defined $help )
{
    # If the user requested help, print the usage notes and exit.
    usage();
    exit;
}

if ( ! defined $suffix )
{
    # The user didn't supply a suffix, so append nothing to the file
    # names.
    $suffix = "";
}
else
{
    # Otherwise, stick a "-" before the suffix, so that a suffix of
    # "test" applied to filename.gdml becomes "filename-test.gdml".
    $suffix = "-" . $suffix;
}


$workspace = 0;

if (defined $wkspc ) # not done 
{
    $workspace = $wkspc;
}
elsif ( $workspace != 0 )
{
    print "\t\tCreating smaller workspace geometry.\n";
}

# set wires on to be the default, unless given an input by the user
$wires_on = 1; # 1=on, 0=off
if (defined $wires)
{
    $wires_on = $wires
}

$tpc_on = 1;

$basename = "dunedphase10kt_v2";
if ( $workspace == 1 )
{
    $basename = $basename."_workspace";
}
if ( $workspace == 2 )
{
    $basename = $basename."_workspace4x2";
}

if ( $wires_on == 0 )
{
    $basename = $basename."_nowires";
}



##################################################################
############## Parameters for Charge Readout Plane ###############

# dune10kt dual-phase
$wirePitch           = 0.3125;  # channel pitch
$nChannelsViewPerCRM = 960;     # channels per collection view
$borderCRM           = 0.5;     # dead space at the border of each CRM

# dimensions of a single Charge Readout Module (CRM)
$widthCRM_active  = $wirePitch * $nChannelsViewPerCRM;
$lengthCRM_active = $wirePitch * $nChannelsViewPerCRM;

$widthCRM  = $widthCRM_active + 2 * $borderCRM;
$lengthCRM = $lengthCRM_active + 2 * $borderCRM;

# number of CRMs in y and z
$nCRM_y   = 4;
$nCRM_z   = 20;

# create a smaller geometry
if( $workspace == 1 )
{
    $nCRM_y = 1;
    $nCRM_z = 2;
}

# create a smaller geometry
if( $workspace == 2 )
{
    $nCRM_y = 2;
    $nCRM_z = 4;
}

# calculate tpc area based on number of CRMs and their dimensions
$widthTPCActive  = $nCRM_y * $widthCRM;  # around 1200
$lengthTPCActive = $nCRM_z * $lengthCRM; # around 6000

# active volume dimensions 
$driftTPCActive  = 1200.0;

# model anode strips as wires
$padWidth  = 0.015;
$ReadoutPlane = 2 * $padWidth;

#$padHeight = 0.0035; 

##################################################################
############## Parameters for TPC and inner volume ###############

# inner volume dimensions of the cryostat
$Argon_x = 1510;
$Argon_y = 1510;
$Argon_z = 6200;

if( $workspace != 0 )
{
    #active tpc + 1 m buffer on each side
    $Argon_y = $widthTPCActive + 200;
    $Argon_z = $lengthTPCActive + 200;
}

# width of gas argon layer on top
$HeightGaseousAr = 100;

# size of liquid argon buffer
$xLArBuffer = $Argon_x - $driftTPCActive - $HeightGaseousAr - $ReadoutPlane;
$yLArBuffer = 0.5 * ($Argon_y - $widthTPCActive);
$zLArBuffer = 0.5 * ($Argon_z - $lengthTPCActive);

# cryostat 
$SteelThickness = 0.12; # membrane

$Cryostat_x = $Argon_x + 2*$SteelThickness;
$Cryostat_y = $Argon_y + 2*$SteelThickness;
$Cryostat_z = $Argon_z + 2*$SteelThickness;

##################################################################
############## DetEnc and World relevant parameters  #############

$SteelSupport_x  =  100;
$SteelSupport_y  =  50;
$SteelSupport_z  =  100; 
$FoamPadding     =  80;  # only 2 layers ???
$FracMassOfSteel =  0.5; #The steel support is not a solid block, but a mixture of air and steel
$FracMassOfAir   =  1 - $FracMassOfSteel;


$SpaceSteelSupportToWall    = 100;
$SpaceSteelSupportToCeiling = 100;

$DetEncWidth   =    $Cryostat_x
                  + 2*($SteelSupport_x + $FoamPadding) + 2*$SpaceSteelSupportToWall;
$DetEncHeight  =    $Cryostat_y
                  + 2*($SteelSupport_y + $FoamPadding) + $SpaceSteelSupportToCeiling;
$DetEncLength  =    $Cryostat_z
                  + 2*($SteelSupport_z + $FoamPadding) + 2*$SpaceSteelSupportToWall;

$posCryoInDetEnc_y = - $DetEncHeight/2 + $SteelSupport_y + $FoamPadding + $Cryostat_y/2;

$RockThickness = 3000;

  # We want the world origin to be at the very front of the fiducial volume.
  # move it to the front of the enclosure, then back it up through the concrete/foam, 
  # then through the Cryostat shell, then through the upstream dead LAr (including the
  # dead LAr on the edge of the TPC)
  # This is to be added to the z position of every volume in volWorld

$OriginZSet =   $DetEncLength/2.0 
              - $SpaceSteelSupportToWall
              - $SteelSupport_z
              - $FoamPadding
              - $SteelThickness
              - $zLArBuffer;
              - $borderCRM;

  # We want the world origin to be vertically centered on active TPC
  # This is to be added to the y position of every volume in volWorld

$OriginYSet =   $DetEncHeight/2.0
              - $SteelSupport_y
              - $FoamPadding
              - $SteelThickness
              - $yLArBuffer
              - $widthTPCActive/2.0;

$OriginXSet =  $DetEncWidth/2.0
              -$SpaceSteelSupportToWall
              -$SteelSupport_x
              -$FoamPadding
              -$SteelThickenss
              -$xLArBuffer
              -$driftTPCActive/2.0;


##################################################################
############## Field Cage Parameters ###############

$FieldShaperLongTubeLength =  $lengthTPCActive;
$FieldShaperShortTubeLength =  $widthTPCActive;
$FieldShaperInnerRadious = 1.485;
$FieldShaperOuterRadious = 1.685;
$FieldShaperTorRad = 1.69;

$FieldShaperLength = $FieldShaperLongTubeLength + 2*$FieldShaperOuterRadious+ 2*$FieldShaperTorRad;
$FieldShaperWidth =  $FieldShaperShortTubeLength + 2*$FieldShaperOuterRadious+ 2*$FieldShaperTorRad;

$FieldShaperSeparation = 5.0;
$NFieldShapers = ($driftTPCActive/$FieldShaperSeparation) - 1;

$FieldCageSizeX = $FieldShaperSeparation*$NFieldShapers+2;
$FieldCageSizeY = $FieldShaperWidth+2;
$FieldCageSizeZ = $FieldShaperLength+2;


##################################################################
############## Parameters for PMTs ###############
$HeightPMT = 37.0;
$pmtNy=int(0.01*$widthTPCActive);
$pmtNz=int(0.01*$lengthTPCActive);
$pmtN = $pmtNy*$pmtNz; #keeping 1m2 density of pmts
$posPMTx=0;

$posPMTx =  -$Argon_x/2 + 0.5*($HeightPMT);
$posPMTx =-$OriginXSet+(-1-$NFieldShapers*0.5)*$FieldShaperSeparation - 100 - 0.5*($HeightPMT); #1m below the cathode


@pmt_pos = ('','','');

$pmt_pos[$pmtN]='';
$counter=0;


for($i=0;$i<$pmtNy;++$i)
{
	for($j=0;$j<$pmtNz;++$j)
	{
	$pmt_pos[$counter]="x=\"$posPMTx\" y=\"@{[-0.5*($pmtNy-1)*100 + $i*100]}\" z=\"@{[-0.5*($pmtNz-1)*100+$j*100]}\"";
	$counter++;
	}	
}
print "Number of PMTs      : $counter (@{[$pmtNy]} x @{[$pmtNz]})\n";
print "widthTPCActive       : $widthTPCActive \n";
##################################################################
############### Parameters for det elements ######################

# cathode plane
#$Cathode_x = $widthTPCActive;
#$Cathode_y = 1.0;
#$Cathode_z = $lengthTPCActive;



#+++++++++++++++++++++++++ End defining variables ++++++++++++++++++++++++++


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++ usage +++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub usage()
{
    print "Usage: $0 [-h|--help] [-o|--output <fragments-file>] [-s|--suffix <string>]\n";
    print "       if -o is omitted, output goes to STDOUT; <fragments-file> is input to make_gdml.pl\n";
    print "       -s <string> appends the string to the file names; useful for multiple detector versions\n";
    print "       -h prints this message, then quits\n";
}



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_Define +++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_Define()
{

# Create the <define> fragment file name, 
# add file to list of fragments,
# and open it
    $DEF = $basename."_Def" . $suffix . ".gdml";
    push (@gdmlFiles, $DEF);
    $DEF = ">" . $DEF;
    open(DEF) or die("Could not open file $DEF for writing");


print DEF <<EOF;
<?xml version='1.0'?>
<gdml>
<define>

<!--



-->

   <position name="posCryoInDetEnc"     unit="cm" x="0" y="$posCryoInDetEnc_y" z="0"/>
   <position name="posCenter"           unit="cm" x="0" y="0" z="0"/>
   <rotation name="rPlus90AboutX"       unit="deg" x="90" y="0" z="0"/>
   <rotation name="rMinus90AboutY"      unit="deg" x="0" y="270" z="0"/>
   <rotation name="rMinus90AboutYMinus90AboutX"       unit="deg" x="270" y="270" z="0"/>
   <rotation name="rPlus180AboutX"	unit="deg" x="180" y="0"   z="0"/>
   <rotation name="rPlus180AboutY"	unit="deg" x="0" y="180"   z="0"/>
   <rotation name="rPlus180AboutXPlus180AboutY"	unit="deg" x="180" y="180"   z="0"/>
   <rotation name="rIdentity"		unit="deg" x="0" y="0"   z="0"/>
</define>
</gdml>
EOF
    close (DEF);
}

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++ gen_Materials +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_Materials()
{

# Create the <materials> fragment file name,
# add file to list of output GDML fragments,
# and open it
    $MAT = $basename."_Materials" . $suffix . ".gdml";
    push (@gdmlFiles, $MAT);
    $MAT = ">" . $MAT;

    open(MAT) or die("Could not open file $MAT for writing");

    # Add any materials special to this geometry by defining a mulitline string
    # and passing it to the gdmlMaterials::gen_Materials() function.
my $asmix = <<EOF;
  <!-- preliminary values -->
  <material name="AirSteelMixture" formula="AirSteelMixture">
   <D value=" 0.001205*(1-$FracMassOfSteel) + 7.9300*$FracMassOfSteel " unit="g/cm3"/>
   <fraction n="$FracMassOfSteel" ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
   <fraction n="$FracMassOfAir"   ref="Air"/>
  </material>
EOF

    # add the general materials used anywere
    print MAT gdmlMaterials::gen_Materials( $asmix );

    close(MAT);
}

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++++ gen_TPC ++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
sub gen_TPC()
{
    # CRM active volume
    my $TPCActive_x = $driftTPCActive;
    my $TPCActive_y = $widthCRM_active;
    my $TPCActive_z = $lengthCRM_active;

    # CRM total volume
    my $TPC_x = $TPCActive_x + $ReadoutPlane;
    my $TPC_y = $widthCRM;
    my $TPC_z = $lengthCRM;

	
    $TPC = $basename."_TPC" . $suffix . ".gdml";
    push (@gdmlFiles, $TPC);
    $TPC = ">" . $TPC;
    open(TPC) or die("Could not open file $TPC for writing");

# The standard XML prefix and starting the gdml
    print TPC <<EOF;
<?xml version='1.0'?>
<gdml>
EOF

    
    # All the TPC solids save the wires.
    print TPC <<EOF;
<solids>
   <box name="CRM" lunit="cm" 
      x="$TPC_x" 
      y="$TPC_y" 
      z="$TPC_z"/>
   <box name="CRMVPlane" lunit="cm" 
      x="$padWidth" 
      y="$TPCActive_y" 
      z="$TPCActive_z"/>
   <box name="CRMZPlane" lunit="cm" 
      x="$padWidth"
      y="$TPCActive_y"
      z="$TPCActive_z"/>
   <box name="CRMActive" lunit="cm"
      x="$TPCActive_x"
      y="$TPCActive_y"
      z="$TPCActive_z"/>
EOF


#++++++++++++++++++++++++++++ Wire Solids ++++++++++++++++++++++++++++++
# in principle we only need only one wire solid, since CRM is a square 
# but to be more general ...

print TPC <<EOF;

   <tube name="CRMWireV"
     rmax="0.5*$padWidth"
     z="$TPCActive_z"               
     deltaphi="360"
     aunit="deg"
     lunit="cm"/>
   <tube name="CRMWireZ"
     rmax="0.5*$padWidth"
     z="$TPCActive_y"               
     deltaphi="360"
     aunit="deg"
     lunit="cm"/>
</solids>

EOF


# Begin structure and create wire logical volumes
print TPC <<EOF;
<structure>
    <volume name="volTPCActive">
      <materialref ref="LAr"/>
      <solidref ref="CRMActive"/>
    </volume>
EOF

if ($wires_on==1) 
{ 
  print TPC <<EOF;
    <volume name="volTPCWireV">
      <materialref ref="Copper_Beryllium_alloy25"/>
      <solidref ref="CRMWireV"/>
    </volume>

    <volume name="volTPCWireZ">
      <materialref ref="Copper_Beryllium_alloy25"/>
      <solidref ref="CRMWireZ"/>
    </volume>
EOF
}

print TPC <<EOF;

    <volume name="volTPCPlaneV">
      <materialref ref="LAr"/>
      <solidref ref="CRMVPlane"/>
EOF

if ($wires_on==1) # add wires to Z plane
{
for($i=0;$i<$nChannelsViewPerCRM;++$i)
{
my $ypos = -0.5 * $TPCActive_y + $i*$wirePitch + 0.5*$padWidth;

print TPC <<EOF;
    <physvol>
      <volumeref ref="volTPCWireV"/> 
      <position name="posWireV$i" unit="cm" x="0" y="$ypos" z="0"/>
      <rotationref ref="rIdentity"/> 
    </physvol>
EOF
}
}

print TPC <<EOF;
   </volume>
   
   <volume name="volTPCPlaneZ">
     <materialref ref="LAr"/>
     <solidref ref="CRMZPlane"/>
EOF


if ($wires_on==1) # add wires to X plane
{
for($i=0;$i<$nChannelsViewPerCRM;++$i)
{

my $zpos = -0.5 * $TPCActive_z + $i*$wirePitch + 0.5*$padWidth;
print TPC <<EOF;
    <physvol>
     <volumeref ref="volTPCWireZ"/>
     <position name="posWireZ$i" unit="cm" x="0" y="0" z="$zpos"/>
     <rotationref ref="rPlus90AboutX"/>
    </physvol>
EOF
}
}


print TPC <<EOF;
   </volume>
EOF


$posVplane[0] = 0.5*$TPC_x - 1.5*$padWidth;
$posVplane[1] = 0;
$posVplane[2] = 0;

$posZplane[0] = 0.5*$TPC_x - 0.5*$padWidth;
$posZplane[1] = 0; 
$posZplane[2] = 0;

$posTPCActive[0] = -$ReadoutPlane;
$posTPCActive[1] = 0;
$posTPCActive[2] = 0;


#wrap up the TPC file
print TPC <<EOF;

   <volume name="volTPC">
     <materialref ref="LAr"/>
     <solidref ref="CRM"/>
     <physvol>
       <volumeref ref="volTPCPlaneV"/>
       <position name="posPlaneV" unit="cm" 
         x="$posVplane[0]" y="$posVplane[1]" z="$posVplane[2]"/>
       <rotationref ref="rIdentity"/>
     </physvol>
     <physvol>
       <volumeref ref="volTPCPlaneZ"/>
       <position name="posPlaneZ" unit="cm" 
         x="$posZplane[0]" y="$posZplane[1]" z="$posZplane[2]"/>
       <rotationref ref="rIdentity"/>
     </physvol>
     <physvol>
       <volumeref ref="volTPCActive"/>
       <position name="posActive" unit="cm" 
         x="@{[$posTPCActive[0]+0.015]}" y="$posTPCActive[1]" z="$posTPCActive[2]"/>
       <rotationref ref="rIdentity"/>
     </physvol>
   </volume>
EOF


print TPC <<EOF;
</structure>
</gdml>
EOF

close(TPC);
}





#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_FieldCage +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_FieldCage {

    $FieldCage = $basename."_FieldCage" . $suffix . ".gdml";
    push (@gdmlFiles, $FieldCage);
    $FieldCage = ">" . $FieldCage;
    open(FieldCage) or die("Could not open file $FieldCage for writing");

# The standard XML prefix and starting the gdml
print FieldCage <<EOF;
   <?xml version='1.0'?>
   <gdml>
EOF
# The printing solids used in the Field Cage
#print "lengthTPCActive      : $lengthTPCActive \n";
#print "widthTPCActive       : $widthTPCActive \n";


print FieldCage <<EOF;
<solids>
     <torus name="FieldShaperCorner" rmin="$FieldShaperInnerRadious" rmax="$FieldShaperOuterRadious" rtor="$FieldShaperTorRad" deltaphi="90" startphi="0" aunit="deg" lunit="cm"/>
     <tube name="FieldShaperLongtube" rmin="$FieldShaperInnerRadious" rmax="$FieldShaperOuterRadious" z="$FieldShaperLongTubeLength" deltaphi="360" startphi="0" aunit="deg" lunit="cm"/>
     <tube name="FieldShaperShorttube" rmin="$FieldShaperInnerRadious" rmax="$FieldShaperOuterRadious" z="$FieldShaperShortTubeLength" deltaphi="360" startphi="0" aunit="deg" lunit="cm"/>

    <union name="FSunion1">
      <first ref="FieldShaperLongtube"/>
      <second ref="FieldShaperCorner"/>
   		<position name="esquinapos1" unit="cm" x="@{[-$FieldShaperTorRad]}" y="0" z="@{[0.5*$FieldShaperLongTubeLength]}"/>
		<rotation name="rot1" unit="deg" x="90" y="0" z="0" />
    </union>

    <union name="FSunion2">
      <first ref="FSunion1"/>
      <second ref="FieldShaperShorttube"/>
   		<position name="esquinapos2" unit="cm" x="@{[-0.5*$FieldShaperShortTubeLength-$FieldShaperTorRad]}" y="0" z="@{[+0.5*$FieldShaperLongTubeLength+$FieldShaperTorRad]}"/>
   		<rotation name="rot2" unit="deg" x="0" y="90" z="0" />
    </union>

    <union name="FSunion3">
      <first ref="FSunion2"/>
      <second ref="FieldShaperCorner"/>
   		<position name="esquinapos3" unit="cm" x="@{[-$FieldShaperShortTubeLength-$FieldShaperTorRad]}" y="0" z="@{[0.5*$FieldShaperLongTubeLength]}"/>
		<rotation name="rot3" unit="deg" x="90" y="270" z="0" />
    </union>

    <union name="FSunion4">
      <first ref="FSunion3"/>
      <second ref="FieldShaperLongtube"/>
   		<position name="esquinapos4" unit="cm" x="@{[-$FieldShaperShortTubeLength-2*$FieldShaperTorRad]}" y="0" z="0"/>
    </union>

    <union name="FSunion5">
      <first ref="FSunion4"/>
      <second ref="FieldShaperCorner"/>
   		<position name="esquinapos5" unit="cm" x="@{[-$FieldShaperShortTubeLength-$FieldShaperTorRad]}" y="0" z="@{[-0.5*$FieldShaperLongTubeLength]}"/>
		<rotation name="rot5" unit="deg" x="90" y="180" z="0" />
    </union>

    <union name="FSunion6">
      <first ref="FSunion5"/>
      <second ref="FieldShaperShorttube"/>
   		<position name="esquinapos6" unit="cm" x="@{[-0.5*$FieldShaperShortTubeLength-$FieldShaperTorRad]}" y="0" z="@{[-0.5*$FieldShaperLongTubeLength-$FieldShaperTorRad]}"/>
		<rotation name="rot6" unit="deg" x="0" y="90" z="0" />
    </union>

    <union name="FieldShaperSolid">
      <first ref="FSunion6"/>
      <second ref="FieldShaperCorner"/>
   		<position name="esquinapos7" unit="cm" x="@{[-$FieldShaperTorRad]}" y="0" z="@{[-0.5*$FieldShaperLongTubeLength]}"/>
		<rotation name="rot7" unit="deg" x="90" y="90" z="0" />
    </union>
</solids>

EOF

print FieldCage <<EOF;

<structure>
<volume name="volFieldShaper">
  <materialref ref="Al2O3"/>
  <solidref ref="FieldShaperSolid"/>
</volume>
</structure>

EOF

print FieldCage <<EOF;

</gdml>
EOF
close(FieldCage);
}

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_ExtractionGrid +++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_ExtractionGrid {

    $ExtractionGrid = $basename."_ExtractionGrid" . $suffix . ".gdml";
    push (@gdmlFiles, $ExtractionGrid);
    $ExtractionGrid = ">" . $ExtractionGrid;
    open(ExtractionGrid) or die("Could not open file $ExtractionGrid for writing");

# The standard XML prefix and starting the gdml
print ExtractionGrid <<EOF;
<?xml version='1.0'?>
<gdml>
EOF

#GroundGrid SOLIDS
$ExtractionGridRadious = 0.05;
$ExtractionGridPitch = 0.3;

$ExtractionGridSizeX = 2*$ExtractionGridRadious;
$ExtractionGridSizeY = $widthTPCActive;
$ExtractionGridSizeZ = $lengthTPCActive;


print ExtractionGrid <<EOF;

<solids>
      <tube name="solExtractionGridCable" rmin="0" rmax="$ExtractionGridRadious" z="$ExtractionGridSizeZ" deltaphi="360" startphi="0" aunit="deg" lunit="cm"/>
     <box name="solExtractionGrid" x="@{[$ExtractionGridSizeX]}" y="$ExtractionGridSizeY" z="@{[$ExtractionGridSizeZ]}" lunit="cm"/>
</solids>

EOF


print ExtractionGrid <<EOF;

<structure>

<volume name="volExtractionGridCable">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="solExtractionGridCable"/>
</volume>

<volume name="volExtractionGrid">
  <materialref ref="LAr"/>
  <solidref ref="solExtractionGrid"/>
EOF

for($ii=0;$ii<$$ExtractionGridSizeY;$ii=$ii+$ExtractionGridPitch)
{
	print ExtractionGrid <<EOF;
  <physvol>
   <volumeref ref="volExtractionGridCable"/>
   <position name="posExtractionGridCable$ii" unit="cm" x="0" y="@{[$ii-0.5*$ExtractionGridSizeY]}" z="0"/>
   <rotation name="GGrot$aux2" unit="deg" x="0" y="90" z="0" /> 
   </physvol>
EOF
 
}

for($jj=0;$jj<$$ExtractionGridSizeZ;$jj=$jj+$ExtractionGridPitch)
{
	print ExtractionGrid <<EOF;
  <physvol>
   <volumeref ref="volExtractionGridCable"/>
   <position name="posExtractionGridCableLat$jj" unit="cm" x="0" y="0" z="@{[$jj-0.5*$ExtractionGridSizeZ]}"/>
   <rotation name="GGrot$aux2" unit="deg" x="90" y="0" z="0" /> 
   </physvol>
EOF
 
}

	print ExtractionGrid <<EOF;
  
  </volume>
</structure>
</gdml>
EOF
close(ExtractionGrid);
}


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_LEMs +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_LEMs {


    $LEMs = $basename."_LEMs" . $suffix . ".gdml";
    push (@gdmlFiles, $LEMs);
    $LEMs = ">" . $LEMs;
    open(LEMs) or die("Could not open file $LEMs for writing");

# The standard XML prefix and starting the gdml
print LEMs <<EOF;
<?xml version='1.0'?>
<gdml>
EOF

$LEMsSizeX=0.1;
$LEMsSizeY=$widthTPCActive;
$LEMsSizeZ=$lengthTPCActive;

print LEMs <<EOF;

<solids>
     <box name="solLEMs" x="@{[$LEMsSizeX]}" y="$LEMsSizeY" z="@{[$LEMsSizeZ]}" lunit="cm"/>

</solids>

EOF
print LEMs <<EOF;


<structure>
 <volume name="volLEMs">
  <materialref ref="Copper_Beryllium_alloy25"/>
  <solidref ref="solLEMs"/>
 </volume>
</structure>
</gdml>
EOF
close(LEMs);
}

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_GroundGrid +++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_GroundGrid {

    $GroundGrid = $basename."_GroundGrid" . $suffix . ".gdml";
    push (@gdmlFiles, $GroundGrid);
    $GroundGrid = ">" . $GroundGrid;
    open(GroundGrid) or die("Could not open file $GroundGrid for writing");

# The standard XML prefix and starting the gdml
print GroundGrid <<EOF;
<?xml version='1.0'?>
<gdml>
EOF

#GroundGrid SOLIDS
$GroundGridSizeX = $FieldShaperWidth+2;
$GroundGridSizeY = 2*$FieldShaperOuterRadious+1;
$GroundGridSizeZ = $FieldShaperLength+2;

$GroundGridInnerStructureLength = $widthTPCActive-1;
$GroundGridInnerStructureLengthLat = $lengthTPCActive;
$GroundGridInnerStructureWidth = 2;
$GroundGridInnerStructureHeight = 4;
$GroundGridInnerStructureSeparation = 65.0;
$GroundGridInnerStructureNumberOfBars = int($lengthTPCActive/$GroundGridInnerStructureSeparation - 1);
$GroundGridInnerStructureNumberOfBarsLat = int($widthTPCActive/$GroundGridInnerStructureSeparation - 1);
#print "number of bars $GroundGridInnerStructureNumberOfBars";

$GroundGridInnerStructureNumberOfCablesPerInnerSquare = 5.0;
$GroundGridInnerStructureCableRadious = 0.1;
$GroundGridInnerStructureCableSeparation = $GroundGridInnerStructureSeparation/($GroundGridInnerStructureNumberOfCablesPerInnerSquare+1);

print GroundGrid <<EOF;

<solids>
     <box name="GroundGridInnerBox" x="@{[$GroundGridInnerStructureWidth]}" y="$GroundGridInnerStructureHeight" z="@{[$GroundGridInnerStructureLength]}" lunit="cm"/>    
     <box name="GroundGridInnerBoxLat" x="@{[$GroundGridInnerStructureWidth]}" y="$GroundGridInnerStructureHeight" z="@{[$GroundGridInnerStructureLengthLat]}" lunit="cm"/>    

     <tube name="GroundGridCable" rmin="0" rmax="$GroundGridInnerStructureCableRadious" z="@{[$GroundGridInnerStructureLength]}" deltaphi="360" startphi="0"  aunit="deg" lunit="cm"/>

<box name="GroundGridModule" x="@{[$GroundGridSizeX]}" y="$GroundGridSizeY"    z="@{[$GroundGridSizeZ]}" lunit="cm"/>

</solids>

EOF

print GroundGrid <<EOF;

<structure>

<volume name="volGroundGridCable">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="GroundGridCable"/>
</volume>

<volume name="volGroundGridInnerBox">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="GroundGridInnerBox"/>
</volume>
<volume name="volGroundGridInnerBoxLat">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="GroundGridInnerBoxLat"/>
</volume>

<volume name="volGGunion">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="FieldShaperSolid"/>
</volume>

 <volume name="volGroundGrid">
  <materialref ref="LAr"/>
  <solidref ref="GroundGridModule"/>

  <physvol>
   <volumeref ref="volGGunion"/>
   <position name="posGGunion" unit="cm" x="@{[0.5*$GroundGridSizeX-0.5*$FieldShaperOuterRadious-1]}" y="@{[0.0]}" z="@{[0.0]}"/>
  </physvol>

EOF

$shift = $GroundGridSizeZ - ($GroundGridInnerStructureNumberOfBars+1)*$GroundGridInnerStructureSeparation;

for($ii=0;$ii<$GroundGridInnerStructureNumberOfBars;$ii++)
{
	print GroundGrid <<EOF;
  <physvol>
   <volumeref ref="volGroundGridInnerBox"/>
   <position name="posGGInnerBox$ii" unit="cm" x="@{[0.6]}" y="@{[0]}" z="@{[0.5*$shift-0.5*$GroundGridSizeZ+($ii+1)*$GroundGridInnerStructureSeparation]}"/>
   <rotation name="rotGG$ii" unit="deg" x="0" y="90" z="0" /> 
  </physvol>
EOF

}

for($ii=-1;$ii<$GroundGridInnerStructureNumberOfBars;$ii++)
{
 for($jj=0;$jj<$GroundGridInnerStructureNumberOfCablesPerInnerSquare;$jj++)
 {
	print GroundGrid <<EOF;
  <physvol>
   <volumeref ref="volGroundGridCable"/>
   <position name="posGGCable$ii$jj" unit="cm" x="@{[0]}" y="@{[0]}" z="@{[0.5*$shift-0.5*$GroundGridSizeZ+($ii+1)*$GroundGridInnerStructureSeparation + ($jj+1)*$GroundGridInnerStructureCableSeparation]}"/>
   <rotation name="rotGGcable$ii$jj" unit="deg" x="0" y="90" z="0" /> 

  </physvol>
EOF
 }
   
}

$shift = $GroundGridSizeX - ($GroundGridInnerStructureNumberOfBarsLat+1)*$GroundGridInnerStructureSeparation;

for($ii=0;$ii<$GroundGridInnerStructureNumberOfBarsLat;$ii++)
{
	print GroundGrid <<EOF;
  <physvol>
   <volumeref ref="volGroundGridInnerBoxLat"/>
   <position name="posGGInnerBoxLat$ii" unit="cm" x="@{[0.5*$shift-0.5*$GroundGridSizeX+($ii+1)*$GroundGridInnerStructureSeparation]}" y="0" z="@{[0.0]}"/>

   </physvol>
EOF
    $aux++;   
}
	print GroundGrid <<EOF;
  
  </volume>
</structure>
</gdml>
EOF
close(GroundGrid);
}






#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_pmt +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_pmt {


#PMTs

#$PMT_COATING_THICKNESS=0.2;
#$PMT_PLATE_THICKNESS=0.4;
#$PMT_GLASS_THICKNESS=0.2;

    $PMT = $basename."_PMT" . $suffix . ".gdml";
    push (@gdmlFiles, $PMT);
    $PMT = ">" . $PMT;
    open(PMT) or die("Could not open file $PMT for writing");

# The standard XML prefix and starting the gdml
print PMT <<EOF;
   <?xml version='1.0'?>
   <gdml>
EOF
	print PMT <<EOF;

<solids>
 <tube name="PMTVolume"
  rmax="(6.5*2.54)"
  z="(11.1*2.54)"
  deltaphi="360"
  aunit="deg"
  lunit="cm"/>

 <tube name="PMT_AcrylicPlate"
  rmax="11.4"
  z="0.4"
  deltaphi="360"
  aunit="deg"
  lunit="cm"/>

 <tube name="PMT_plate_coat"
  rmax="11.4"
  z="0.02"
  deltaphi="360"
  aunit="deg"
  lunit="cm"/>


    <tube aunit="deg" deltaphi="360" lunit="mm" name="pmtMiddleCylinder" rmax="102.351822048586" rmin="100.351822048586" startphi="0" z="54"/>
    <sphere aunit="deg" deltaphi="360" deltatheta="50" lunit="mm" name="sphPartTop" rmax="133" rmin="131" startphi="0" starttheta="0"/>
    <union name="pmt0x7fb8f489dfe0">
      <first ref="pmtMiddleCylinder"/>
      <second ref="sphPartTop"/>
      <position name="pmt0x7fb8f489dfe0_pos" unit="mm" x="0" y="0" z="-57.2051768689367"/>
    </union>
    <sphere aunit="deg" deltaphi="360" deltatheta="31.477975238527" lunit="mm" name="sphPartBtm" rmax="133" rmin="131" startphi="0" starttheta="130"/>
    <union name="pmt0x7fb8f48a0d50">
      <first ref="pmt0x7fb8f489dfe0"/>
      <second ref="sphPartBtm"/>
      <position name="pmt0x7fb8f48a0d50_pos" unit="mm" x="0" y="0" z="57.2051768689367"/>
    </union>
    <tube aunit="deg" deltaphi="360" lunit="mm" name="pmtBtmTube" rmax="44.25" rmin="42.25" startphi="0" z="72"/>
    <union name="solidpmt">
      <first ref="pmt0x7fb8f48a0d50"/>
      <second ref="pmtBtmTube"/>
      <position name="solidpmt_pos" unit="mm" x="0" y="0" z="-104.905637496842"/>
    </union>
    <sphere aunit="deg" deltaphi="360" deltatheta="50" lunit="mm" name="pmt0x7fb8f48a1eb0" rmax="133.2" rmin="133" startphi="0" starttheta="0"/>
    <sphere aunit="deg" deltaphi="360" deltatheta="46.5" lunit="mm" name="pmt0x7fb8f48a4860" rmax="131" rmin="130.999" startphi="0" starttheta="0"/>


</solids>

<structure>


 <volume name="pmtCoatVol">
  <materialref ref="LAr"/>
  <solidref ref="pmt0x7fb8f48a1eb0"/>
  </volume>

 <volume name="allpmt">
  <materialref ref="Glass"/>
  <solidref ref="solidpmt"/>
  </volume>

<volume name="volPMT">
  <materialref ref="LAr"/>
  <solidref ref="PMTVolume"/>

  <physvol>
   <volumeref ref="allpmt"/>
   <position name="posallpmtcoat" unit="cm" x="0" y="0" z="@{[1.27*2.54]}"/>
  </physvol>

 <physvol name="volOpDetSensitiveCoat">
  <volumeref ref="pmtCoatVol"/>
  <position name="posOpDetSensitiveCoat" unit="cm" x="0" y="0" z="@{[1.27*2.54- (2.23*2.54)]}"/>
  </physvol>

 </volume>

</structure>
</gdml>

EOF
}



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++++ gen_Cryostat +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_Cryostat()
{

# Create the cryostat fragment file name,
# add file to list of output GDML fragments,
# and open it
    $CRYO = $basename."_Cryostat" . $suffix . ".gdml";
    push (@gdmlFiles, $CRYO);
    $CRYO = ">" . $CRYO;
    open(CRYO) or die("Could not open file $CRYO for writing");


# The standard XML prefix and starting the gdml
    print CRYO <<EOF;
<?xml version='1.0'?>
<gdml>
EOF

# All the cryostat solids.
print CRYO <<EOF;
<solids>
    <box name="Cryostat" lunit="cm" 
      x="$Cryostat_x" 
      y="$Cryostat_y" 
      z="$Cryostat_z"/>

    <box name="ArgonInterior" lunit="cm" 
      x="$Argon_x"
      y="$Argon_y"
      z="$Argon_z"/>

    <box name="GaseousArgon" lunit="cm" 
      x="$HeightGaseousAr"
      y="$Argon_y"
      z="$Argon_z"/>

    <subtraction name="SteelShell">
      <first ref="Cryostat"/>
      <second ref="ArgonInterior"/>
    </subtraction>

</solids>
EOF

# Cryostat structure
print CRYO <<EOF;
<structure>
    <volume name="volSteelShell">
      <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni" />
      <solidref ref="SteelShell" />
    </volume>
    <volume name="volGaseousArgon">
      <materialref ref="ArGas"/>
      <solidref ref="GaseousArgon"/>
EOF

  if ( $LEMs_switch eq "on" )
  {

$posLEMsX = -0.5*$HeightGaseousAr+0.5-0.5*$LEMsSizeX;
$posLEMsY = 0;
$posLEMsZ = 0;

      print CRYO <<EOF;
      <physvol>
      <volumeref ref="volLEMs"/>
      <position name="posLEMs" unit="cm" x="$posLEMsX" y="$posLEMsY" z="$posLEMsZ"/>
      </physvol>
EOF
  }
      print CRYO <<EOF;
    </volume>

    <volume name="volCryostat">
      <materialref ref="LAr" />
      <solidref ref="Cryostat" />
      <physvol>
        <volumeref ref="volGaseousArgon"/>
        <position name="posGaseousArgon" unit="cm" x="@{[$Argon_x/2-$HeightGaseousAr/2]}" y="0" z="0"/>
      </physvol>
      <physvol>
        <volumeref ref="volSteelShell"/>
        <position name="posSteelShell" unit="cm" x="0" y="0" z="0"/>
      </physvol>
EOF


if ($tpc_on==1) # place TPC inside croysotat
{

  $posX =  $Argon_x/2 - $HeightGaseousAr - 0.5*($driftTPCActive + $ReadoutPlane); 
  for($ii=0;$ii<$nCRM_z;$ii++)
  {
    $posZ = -0.5*$Argon_z + $zLArBuffer + ($ii+0.5)*$lengthCRM;

    for($jj=0;$jj<$nCRM_y;$jj++)
    {
	$posY = -0.5*$Argon_y + $yLArBuffer + ($jj+0.5)*$widthCRM;
	print CRYO <<EOF;
      <physvol>
        <volumeref ref="volTPC"/>
	<position name="posTPC\-$ii\-$jj" unit="cm"
           x="$posX" y="$posY" z="$posZ"/>
      </physvol>
EOF
    }
  }

}

if ( $pmt_switch eq "on" )
{
    for ( $i=0; $i<$pmtN; $i=$i+1 )
    {
	print CRYO <<EOF;
  <physvol>
     <volumeref ref="volPMT"/>
   <position name="posPMT$i" unit="cm" @pmt_pos[$i]/>
   <rotationref ref="rMinus90AboutY"/>
  </physvol>
EOF
    }
}

  if ( $FieldCage_switch eq "on" ) {
    for ( $i=0; $i<$NFieldShapers; $i=$i+1 ) { # pmts with coating
$posX =  $Argon_x/2 - $HeightGaseousAr - 0.5*($driftTPCActive + $ReadoutPlane); 
	print CRYO <<EOF;
  <physvol>
     <volumeref ref="volFieldShaper"/>
     <position name="posFieldShaper$i" unit="cm"  x="@{[-$OriginXSet+($i-$NFieldShapers*0.5)*$FieldShaperSeparation]}" y="@{[-0.5*$FieldShaperShortTubeLength-$FieldShaperTorRad]}" z="0" />
     <rotation name="rotFS$i" unit="deg" x="0" y="0" z="90" />
  </physvol>
EOF
    }
  }

$GroundGridPosX=-$Argon_x/2 + 47.5;
$GroundGridPosX=-$OriginXSet+(-1-$NFieldShapers*0.5)*$FieldShaperSeparation - 80;
$GroundGridPosY=0;

  if ( $GroundGrid_switch eq "on" )
  {
      print CRYO <<EOF;
  <physvol>
   <volumeref ref="volGroundGrid"/>
   <position name="posGroundGrid01" unit="cm" x="$GroundGridPosX" y="@{[-$GroundGridPosY]}" z="@{[$GroundGridPosY]}"/>
   <rotation name="rotGG01" unit="deg" x="0" y="0" z="90" />
  </physvol>

EOF

  }

$CathodePosX=-$OriginXSet+(-1-$NFieldShapers*0.5)*$FieldShaperSeparation;
$CathodePosY=0;
  if ( $Cathode_switch eq "on" )
  {
      print CRYO <<EOF;
  <physvol>
   <volumeref ref="volGroundGrid"/>
   <position name="posGroundGrid01" unit="cm" x="$CathodePosX" y="@{[-$CathodePosY]}" z="@{[$CathodePosY]}"/>
   <rotation name="rotGG01" unit="deg" x="0" y="0" z="90" />
  </physvol>

EOF

  }

  if ( $ExtractionGrid_switch eq "on" )
  {

$ExtractionGridX = 0.5*$Argon_x-$HeightGaseousAr-0.5-0.5*$ExtractionGridSizeX;
$ExtractionGridY = 0;
$ExtractionGridZ = 0;

      print CRYO <<EOF;
  <physvol>
   <volumeref ref="volExtractionGrid"/>
   <position name="posExtractionGrid" unit="cm" x="$ExtractionGridX" y="$ExtractionGridY" z="$ExtractionGridZ"/>

  </physvol>
EOF

  }


print CRYO <<EOF;
    </volume>
</structure>
</gdml>
EOF

close(CRYO);
}



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++ gen_Enclosure +++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_Enclosure()
{

# Create the detector enclosure fragment file name,
# add file to list of output GDML fragments,
# and open it
    $ENCL = $basename."_DetEnclosure" . $suffix . ".gdml";
    push (@gdmlFiles, $ENCL);
    $ENCL = ">" . $ENCL;
    open(ENCL) or die("Could not open file $ENCL for writing");


# The standard XML prefix and starting the gdml
    print ENCL <<EOF;
<?xml version='1.0'?>
<gdml>
EOF


# All the detector enclosure solids.
print ENCL <<EOF;
<solids>

    <box name="FoamPadBlock" lunit="cm"
      x="$Cryostat_x + 2*$FoamPadding"
      y="$Cryostat_y + 2*$FoamPadding"
      z="$Cryostat_z + 2*$FoamPadding" />

    <subtraction name="FoamPadding">
      <first ref="FoamPadBlock"/>
      <second ref="Cryostat"/>
      <positionref ref="posCenter"/>
    </subtraction>

    <box name="SteelSupportBlock" lunit="cm"
      x="$Cryostat_x + 2*$FoamPadding + 2*$SteelSupport_x"
      y="$Cryostat_y + 2*$FoamPadding + 2*$SteelSupport_y"
      z="$Cryostat_z + 2*$FoamPadding + 2*$SteelSupport_z" />

    <subtraction name="SteelSupport">
      <first ref="SteelSupportBlock"/>
      <second ref="FoamPadBlock"/>
      <positionref ref="posCenter"/>
    </subtraction>

    <box name="DetEnclosure" lunit="cm" 
      x="$DetEncWidth"
      y="$DetEncHeight"
      z="$DetEncLength"/>

</solids>
EOF


# Detector enclosure structure
    print ENCL <<EOF;
<structure>
    <volume name="volFoamPadding">
      <materialref ref="fibrous_glass"/>
      <solidref ref="FoamPadding"/>
    </volume>

    <volume name="volSteelSupport">
      <materialref ref="AirSteelMixture"/>
      <solidref ref="SteelSupport"/>
    </volume>

    <volume name="volDetEnclosure">
      <materialref ref="Air"/>
      <solidref ref="DetEnclosure"/>

       <physvol>
           <volumeref ref="volFoamPadding"/>
           <positionref ref="posCryoInDetEnc"/>
       </physvol>
       <physvol>
           <volumeref ref="volSteelSupport"/>
           <positionref ref="posCryoInDetEnc"/>
       </physvol>
       <physvol>
           <volumeref ref="volCryostat"/>
           <positionref ref="posCryoInDetEnc"/>
       </physvol>
EOF


print ENCL <<EOF;
    </volume>
EOF

print ENCL <<EOF;
</structure>
</gdml>
EOF

close(ENCL);
}



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++ gen_World +++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub gen_World()
{

# Create the WORLD fragment file name,
# add file to list of output GDML fragments,
# and open it
    $WORLD = $basename."_World" . $suffix . ".gdml";
    push (@gdmlFiles, $WORLD);
    $WORLD = ">" . $WORLD;
    open(WORLD) or die("Could not open file $WORLD for writing");


# The standard XML prefix and starting the gdml
    print WORLD <<EOF;
<?xml version='1.0'?>
<gdml>
EOF


# All the World solids.
print WORLD <<EOF;
<solids>
    <box name="World" lunit="cm" 
      x="@{[$DetEncWidth+2*$RockThickness]}" 
      y="@{[$DetEncHeight+2*$RockThickness]}" 
      z="@{[$DetEncLength+2*$RockThickness]}"/>
</solids>
EOF

# World structure
print WORLD <<EOF;
<structure>
    <volume name="volWorld" >
      <materialref ref="DUSEL_Rock"/>
      <solidref ref="World"/>

      <physvol>
        <volumeref ref="volDetEnclosure"/>
	<position name="posDetEnclosure" unit="cm" x="$OriginXSet" y="$OriginYSet" z="@{[$OriginZSet-10]}"/>
      </physvol>

    </volume>
</structure>
</gdml>
EOF

# make_gdml.pl will take care of <setup/>

close(WORLD);
}



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#++++++++++++++++++++++++++++++++++++ write_fragments ++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

sub write_fragments()
{
   # This subroutine creates an XML file that summarizes the the subfiles output
   # by the other sub routines - it is the input file for make_gdml.pl which will
   # give the final desired GDML file. Specify its name with the output option.
   # (you can change the name when running make_gdml)

   # This code is taken straigh from the similar MicroBooNE generate script, Thank you.

    if ( ! defined $output )
    {
	$output = "-"; # write to STDOUT 
    }

    # Set up the output file.
    $OUTPUT = ">" . $output;
    open(OUTPUT) or die("Could not open file $OUTPUT");

    print OUTPUT <<EOF;
<?xml version='1.0'?>

<!-- Input to Geometry/gdml/make_gdml.pl; define the GDML fragments
     that will be zipped together to create a detector description. 
     -->

<config>

   <constantfiles>

      <!-- These files contain GDML <constant></constant>
           blocks. They are read in separately, so they can be
           interpreted into the remaining GDML. See make_gdml.pl for
           more information. 
	   -->
	   
EOF

    foreach $filename (@defFiles)
    {
	print OUTPUT <<EOF;
      <filename> $filename </filename>
EOF
    }

    print OUTPUT <<EOF;

   </constantfiles>

   <gdmlfiles>

      <!-- The GDML file fragments to be zipped together. -->

EOF

    foreach $filename (@gdmlFiles)
    {
	print OUTPUT <<EOF;
      <filename> $filename </filename>
EOF
    }

    print OUTPUT <<EOF;

   </gdmlfiles>

</config>
EOF

    close(OUTPUT);
}


print "Some key parameters for dual-phase LAr TPC (unit cm unless noted otherwise)\n";
print "CRM active area    : $widthCRM_active x $lengthCRM_active\n";
print "CRM total area     : $widthCRM x $lengthCRM\n";
print "TPC active volume  : $driftTPCActive x $widthTPCActive x $lengthTPCActive\n";
print "Argon buffer       : ($xLArBuffer, $yLArBuffer, $zLArBuffer) \n"; 
print "Detector enclosure : $DetEncWidth x $DetEncHeight x $DetEncLength\n";
print "TPC Origin         : ($OriginXSet, $OriginYSet, $OriginZSet) \n";
print "Field Cage         : $FieldCage_switch \n";
print "Cathode            : $Cathode_switch \n";;
print "GroundGrid         : $GroundGrid_switch \n";
print "ExtractionGrid     : $ExtractionGrid_switch \n";
print "LEMs               : $LEMs_switch \n";
print "PMTs               : $pmt_switch \n";

# run the sub routines that generate the fragments
if ( $pmt_switch eq "on" ) {  gen_pmt();	}
if ( $FieldCage_switch eq "on" ) {  gen_FieldCage();	}
if ( $GroundGrid_switch eq "on" ) {  gen_GroundGrid();	}
#if ( $Cathode_switch eq "on" ) {  gen_Cathode();	} #Cathode for now has the same geometry as the Ground Grid
if ( $ExtractionGrid_switch eq "on" ) {  gen_ExtractionGrid();	}
if ( $LEMs_switch eq "on" ) {  gen_LEMs();	}


gen_Define(); 	 # generates definitions at beginning of GDML
gen_Materials(); # generates materials to be used
gen_TPC();       # generate TPC for a given unit CRM
gen_Cryostat();  # 
gen_Enclosure(); # 
gen_World();	 # places the enclosure among DUSEL Rock
write_fragments(); # writes the XML input for make_gdml.pl
		   # which zips together the final GDML
print "--- done\n\n\n";
exit;
