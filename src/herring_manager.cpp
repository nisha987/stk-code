//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <stdexcept>
#include <string>
#include <sstream>
#include "preprocessor.hpp"
#include "config.hpp"
#include "herring_manager.hpp"
#include "loader.hpp"
#include "material_manager.hpp"
#include "material.hpp"
#include "kart.hpp"
#include "string_utils.hpp"

// Simple shadow class, only used here for default herrings
class Shadow {
  ssgBranch *sh ;
 
public:
  Shadow ( float x1, float x2, float y1, float y2 ) ;
  ssgEntity *getRoot () { return sh ; }
};   // Shadow

// -----------------------------------------------------------------------------
Shadow::Shadow ( float x1, float x2, float y1, float y2 ) {
  ssgVertexArray   *va = new ssgVertexArray   () ; sgVec3 v ;
  ssgNormalArray   *na = new ssgNormalArray   () ; sgVec3 n ;
  ssgColourArray   *ca = new ssgColourArray   () ; sgVec4 c ;
  ssgTexCoordArray *ta = new ssgTexCoordArray () ; sgVec2 t ;

  sgSetVec4 ( c, 0.0f, 0.0f, 0.0f, 1.0f ) ; ca->add(c) ;
  sgSetVec3 ( n, 0.0f, 0.0f, 1.0f ) ; na->add(n) ;
 
  sgSetVec3 ( v, x1, y1, 0.10f ) ; va->add(v) ;
  sgSetVec3 ( v, x2, y1, 0.10f ) ; va->add(v) ;
  sgSetVec3 ( v, x1, y2, 0.10f ) ; va->add(v) ;
  sgSetVec3 ( v, x2, y2, 0.10f ) ; va->add(v) ;
 
  sgSetVec2 ( t, 0.0f, 0.0f ) ; ta->add(t) ;
  sgSetVec2 ( t, 1.0f, 0.0f ) ; ta->add(t) ;
  sgSetVec2 ( t, 0.0f, 1.0f ) ; ta->add(t) ;
  sgSetVec2 ( t, 1.0f, 1.0f ) ; ta->add(t) ;
 
  sh = new ssgBranch ;
  sh -> clrTraversalMaskBits ( SSGTRAV_ISECT|SSGTRAV_HOT ) ;
 
  sh -> setName ( "Shadow" ) ;
 
  ssgVtxTable *gs = new ssgVtxTable ( GL_TRIANGLE_STRIP, va, na, ta, ca ) ;
 
  gs -> clrTraversalMaskBits ( SSGTRAV_ISECT|SSGTRAV_HOT ) ;
  gs -> setState ( fuzzy_gst ) ;
  sh -> addKid ( gs ) ;
  sh -> ref () ; /* Make sure it doesn't get deleted by mistake */
}   // Shadow

// =============================================================================
HerringManager* herring_manager;

HerringManager::HerringManager() {
    allModels.clear();
  // The actual loading is done in loadDefaultHerrings
}   // HerringManager

// -----------------------------------------------------------------------------
void HerringManager::loadDefaultHerrings() {
  // Load all models. This can't be done in the constructor, since the loader 
  // isn't ready at that stage.

  // Load all models from the models/herrings directory
  // --------------------------------------------------
  std::set<std::string> files;
  loader->listFiles(files, "models/herrings");
  for(std::set<std::string>::iterator i  = files.begin(); 
                                      i != files.end();  ++i) {
    if(!StringUtils::has_suffix(*i, ".ac")) continue;
    std::string fullName  = "herrings/"+(*i);
    ssgEntity*  h         = ssgLoad(fullName.c_str(), loader);
    std::string shortName = StringUtils::without_extension(*i);
    h->ref();
    h->setName(shortName.c_str());
    preProcessObj(h);
    allModels[shortName] = h;
  }   // for i


  // Load the old, internal only models
  // ----------------------------------
  sgVec3 yellow = { 1.0f, 1.0f, 0.4f }; CreateDefaultHerring(yellow, "OLD_GOLD"  );
  sgVec3 cyan   = { 0.4f, 1.0f, 1.0f }; CreateDefaultHerring(cyan  , "OLD_SILVER");
  sgVec3 red    = { 0.8f, 0.0f, 0.0f }; CreateDefaultHerring(red   , "OLD_RED"   );
  sgVec3 green  = { 0.0f, 0.8f, 0.0f }; CreateDefaultHerring(green , "OLD_GREEN" );
  
  setDefaultHerringStyle();
}   // loadDefaultHerrings

// -----------------------------------------------------------------------------
void HerringManager::setDefaultHerringStyle() {
  std::string defaultNames[4] = {"bonusblock", "banana",
				 "goldcoin",   "silvercoin"};

  bool bError=0;
  for(int i=HE_RED; i<=HE_SILVER; i++) {
    herringModel[i] = allModels[defaultNames[i]];
    if(!herringModel[i]) {
      fprintf(stderr, "Herring model '%s' is missing!\n",defaultNames[i].c_str());
      bError=1;
    }   // if !herringModel
  }   // for i
  if(bError) {
    fprintf(stderr, "The following models are available:\n");
    typedef std::map<std::string,ssgEntity*>::const_iterator CI_type;
    for(CI_type i=allModels.begin(); i!=allModels.end(); ++i) {
      if(i->second) {
	if(i->first.substr(0,3)=="OLD") {
	  fprintf(stderr,"   %s internally only.\n",i->first.c_str());
	} else {
	  fprintf(stderr,"   %s in models/herrings/%s.ac.\n",i->first.c_str(),
		  i->first.c_str());
	}
      }  // if i->second
    }
    exit(-1);
  }   // if bError
  
}   // setDefaultHerringStyle

// -----------------------------------------------------------------------------
Herring* HerringManager::newHerring(herringType type, sgVec3* xyz) {
  Herring* h = new Herring(type, xyz, herringModel[type]);
  allHerrings.push_back(h);
  return h;
}   // newHerring

// -----------------------------------------------------------------------------
void  HerringManager::hitHerring(Kart* kart) {
  for(AllHerringType::iterator i =allHerrings.begin(); 
                               i!=allHerrings.end();  i++) {
    if((*i)->wasEaten()) continue;
    if((*i)->hitKart(kart)) {
      (*i)->isEaten();
      kart->collectedHerring(*i);
    }   // if hit
  }   // for allHerrings
}   // hitHerring

// -----------------------------------------------------------------------------
// Remove all herring instances, and the track specific models. This is used
// just before a new track is loaded and a race is started
void HerringManager::cleanup() {
  for(AllHerringType::iterator i =allHerrings.begin(); 
                               i!=allHerrings.end();  i++) {
    delete *i;
  }
  allHerrings.clear();
  
  setDefaultHerringStyle();

  // Then load the default style from the config file
  // ------------------------------------------------
  // This way if a herring is not defined in the herringstyle-file, the
  // default (i.e. old herring) is used.
  loadHerringStyle(config->herringStyle);
  if(userFilename.size()>0) {
    loadHerringStyle(userFilename);
  }

}   // cleanup

// -----------------------------------------------------------------------------
// Remove all herring instances, and the track specific models. This is used
// just before a new track is loaded and a race is started
void HerringManager::reset() {
  for(AllHerringType::iterator i =allHerrings.begin(); 
                               i!=allHerrings.end();  i++) {
    (*i)->reset();
  }  // for i
}   // reset
// -----------------------------------------------------------------------------
void HerringManager::update(float delta) {
  for(AllHerringType::iterator i =allHerrings.begin(); 
                               i!=allHerrings.end();  i++) {
    (*i)->update(delta);
  }   // for allHerrings
}   // delta

// -----------------------------------------------------------------------------
void HerringManager::CreateDefaultHerring(sgVec3 colour, std::string name) {
  ssgVertexArray   *va = new ssgVertexArray   () ; sgVec3 v ;
  ssgNormalArray   *na = new ssgNormalArray   () ; sgVec3 n ;
  ssgColourArray   *ca = new ssgColourArray   () ; sgVec4 c ;
  ssgTexCoordArray *ta = new ssgTexCoordArray () ; sgVec2 t ;
  
  sgSetVec3(v, -0.5f, 0.0f, 0.0f ) ; va->add(v) ;
  sgSetVec3(v,  0.5f, 0.0f, 0.0f ) ; va->add(v) ;
  sgSetVec3(v, -0.5f, 0.0f, 0.5f ) ; va->add(v) ;
  sgSetVec3(v,  0.5f, 0.0f, 0.5f ) ; va->add(v) ;
  sgSetVec3(v, -0.5f, 0.0f, 0.0f ) ; va->add(v) ;
  sgSetVec3(v,  0.5f, 0.0f, 0.0f ) ; va->add(v) ;

  sgSetVec3(n,  0.0f,  1.0f,  0.0f ) ; na->add(n) ;

  sgCopyVec3 ( c, colour ) ; c[ 3 ] = 1.0f ; ca->add(c) ;
 
  sgSetVec2(t, 0.0f, 0.0f ) ; ta->add(t) ;
  sgSetVec2(t, 1.0f, 0.0f ) ; ta->add(t) ;
  sgSetVec2(t, 0.0f, 1.0f ) ; ta->add(t) ;
  sgSetVec2(t, 1.0f, 1.0f ) ; ta->add(t) ;
  sgSetVec2(t, 0.0f, 0.0f ) ; ta->add(t) ;
  sgSetVec2(t, 1.0f, 0.0f ) ; ta->add(t) ;
 

  ssgLeaf *gset = new ssgVtxTable ( GL_TRIANGLE_STRIP, va, na, ta, ca ) ;
 
  gset->setState(material_manager->getMaterial("herring.rgb")->getState()) ;
 
  Shadow* sh = new Shadow ( -0.5f, 0.5f, -0.25f, 0.25f ) ;
 
  ssgTransform* tr = new ssgTransform () ;
 
  tr -> addKid ( sh -> getRoot () ) ;
  tr -> addKid ( gset ) ;
  tr -> ref () ; /* Make sure it doesn't get deleted by mistake */
  preProcessObj(tr);
  allModels[name] = tr;

}   // CreateDefaultHerring
// -----------------------------------------------------------------------------
void HerringManager::loadHerringStyle(const std::string filename) {
  if(filename.length()==0) return;
  const lisp::Lisp* root = 0;
  lisp::Parser parser;
  std::string tmp= "data/" + (std::string)filename + ".herring";
  root = parser.parse(loader->getPath(tmp));

  const lisp::Lisp* herring_node = root->getLisp("herring");
  if(!herring_node) {
    delete root;
    std::stringstream msg;
    msg << "Couldn't load map '" << filename << "': no herring node.";
    throw std::runtime_error(msg.str());
  }

  setHerring(herring_node, "red",   HE_RED   );
  setHerring(herring_node, "green", HE_GREEN );
  setHerring(herring_node, "gold"  ,HE_GOLD  );
  setHerring(herring_node, "silver",HE_SILVER);
}   // loadHerringStyle

// -----------------------------------------------------------------------------
void HerringManager::setHerring(const lisp::Lisp *herring_node, 
				char *colour, herringType type) {
  std::string name;
  herring_node->get(colour, name);
  if(name.size()>0) {
    herringModel[type]=allModels[name];
  }
}   // setHerring
// -----------------------------------------------------------------------------
