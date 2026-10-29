# The newborn: research

The author's research for the newborn's brain, cleaned up from `newborn.pdf`
(38 pages, Oct 2026). The PDF repeated many sections two to four times; each
point appears here once. Nothing has been dropped or simplified. The author's
own design ideas are marked **Idea**. Places where the source was cut off,
unclear or garbled are marked **Open** and collected at the end.

This is the source of truth. Build from this, not from older code.

---

## Part 1. Overall plan

- **Connect the three brains.** Use them to sharpen ideas and check each
  other's results. The three, as built in the earlier chats (Ciall,
  `research/ONE_BRAIN.md`), that did not yet touch each other:
  1. **The grown brain:** anatomy grown from a genome, region by region, with
     the womb's learning written into its synapses (`brain/ob_grow.c`,
     `ob_womb.c`).
  2. **The life brain:** a compact brain that lives: restless brainstem, a
     predicting cerebellum, dopamine from learning progress, basal ganglia
     that learn, a hippocampus that keeps every moment (`brain/ob_life.c`;
     this is what runs in the newborn page today).
  3. **The mind:** baby Ciall's cognition: object map, prediction, invention,
     imagination, intuition, a controller choosing among them, sleep,
     self-correction (`certifiable-c/smarsh_mind.c`).
  The plan was to make them one: the grown brain is the one that lives, and
  the other two become what its regions do. Region by region:

  | Region | In the one brain | From |
  |---|---|---|
  | Retina, cochlea, skin, inner ear (vestibule) | Where the world arrives: what the camera sees, sound bands, touch, being moved | the grown brain |
  | Spinal sensory, at the head | The eye muscles' stretch: where its eyes point | new |
  | Thalamus, four nuclei | Each sense's relay to its cortex; predictions cancel what was expected, and what is left is surprise | the grown brain, the womb |
  | Brainstem | Restless neurons; zones of them are its acts (eyes left, right, up, down, voice, a body jerk); a zone acts when its own activity plus the basal ganglia's push crosses threshold | the life brain |
  | Cerebellum | Efference copies and the senses linger a few moments; its synapses onto the thalamus learn (delta rule) to predict each sense | the womb's touch learning, for every sense |
  | Tegmentum | Dopamine = learning progress, from the thalamus's surprise, per situation | the life brain |
  | Basal ganglia | Hear the cortex (where it looks, what it sees, what it did); learn which urges lead to progress | the mobile's learning, for every act |
  | Hippocampus | Keeps every moment, filed under its own neurons' activity; expects what came next when a moment repeats; learns where that can be trusted | the life brain's memory |
  | Parietal cortex | The map of objects and space | the mind |
  | Frontal cortex | The controller: explore, follow intuition, imagine, ask; self-correction | the mind |
  | Two hemispheres, corpus callosum | Left exact and eliminating (Hartley), right associative (Shannon); the callosum carries between them | later |

  **Phases:** (1) the grown brain lives, and must pass the life brain's checks;
  (2) the mind becomes its cortex, playing the practice worlds through its
  own eyes and acts; (3) the one brain on the phone, grown from its genome.
  The life brain is kept as the sketch to compare against.

  **Where it stood (Oct 22):** phase 1 done; phase 2 step 1 (the cortex
  learns what it sees; entorhinal gateway) and step 2 (the DLPFC reasons by
  Hartley elimination, switched on by the myelination wave: none before 8
  months, 39 hypotheses held at three years) done. The phone still runs the
  life brain, not the one brain. Next was: the being proposes its own
  hypotheses instead of being handed a list.
- **Turn each big idea into a test that could fail.**
- **Let each life stage switch ideas on when the biology does.** Abilities
  appear on the biological timeline, not all at once.
- **Common sense for a baby is not adult common sense.** For a baby it is the
  feeling of touch, and simple motor loops built on it:
  - in the womb, when amniotic fluid touches its lips, the fetus opens its
    mouth and takes it in;
  - occasional kicking, and sucking its thumb;
  - sucking teaches it how to get milk from the breast.
- **No spinal cord.** It is an AI, not a human, so it gets no spinal cord. But
  since it has touch, it needs some motor mechanism that lets it understand
  through touch. (Betz cells, the large pyramidal neurons running from the
  precentral gyrus to the spinal cord, are noted as part of motor
  coordination.)
- **Self-sufficient.** It must develop like a human, without a mother to
  cradle it. Once open-sourced, it should be able to understand where it is.

---

## Part 2. The developmental timeline (embryo to term)

### Stage 1. Molecular induction and neural tube formation (weeks 3–4)
*Embryonic signalling and neurulation*

- **Primary embryonic induction.** Around day 16 the notochord releases
  Noggin, Chordin and Follistatin. These block BMPs (Bone Morphogenetic
  Proteins), which would otherwise turn the tissue into skin. Blocking BMPs
  forces the ectoderm above to become the neural plate.
- **Cell elongation and constriction.** Neural plate cells lengthen through
  microtubule elongation. Microfilaments at the top of the cells contract
  (apical constriction) like a drawstring. This bends the flat plate inward,
  making the neural groove and neural folds.
- **Zipper-like fusion.** The folds rise and meet in the middle (around the
  future neck) and fuse like a zipper, both toward the head (rostrally) and
  toward the spine (caudally). This makes the hollow neural tube.
- **Neuropore closure and failures.** The anterior (top) neuropore closes on
  day 25 and the posterior (bottom) on day 27. If the top fails to close, the
  brain cannot form (anencephaly). If the bottom fails, spina bifida.
- **Vesicle splitting.** Before week 4 ends, the top of the tube becomes three
  primary vesicles: prosencephalon (forebrain), mesencephalon (midbrain) and
  rhombencephalon (hindbrain). By week 5 these become five:
  - forebrain → telencephalon (cerebral cortex, basal ganglia) and
    diencephalon (thalamus, hypothalamus)
  - midbrain stays the mesencephalon
  - hindbrain → metencephalon (pons, cerebellum) and myelencephalon (medulla)

### Stage 2. Cell genesis, kinetics and migration (weeks 5–25)
*Neurogenesis and structural architecture*

- **Ventricular zone birthplace.** All neurons and glia are born in the
  innermost lining of the tube, a layer of stem cells called the ventricular
  zone (VZ).
- **Interkinetic nuclear migration.** As progenitors prepare to divide, their
  nuclei move up and down in the VZ: out to the outer edge to copy DNA, then
  back down beside the fluid-filled ventricle to divide.
- **Symmetric then asymmetric division.** Early on, one stem cell splits into
  two identical stem cells, expanding the pool. Around week 7 they switch to
  asymmetric division: one permanent stem cell plus one immature neuron
  (neuroblast) or glial cell. At the peak, up to 250,000 neurons are made
  every minute.
- **Climbing the radial glia.** Neuroblasts migrate centimetres. Radial glia
  stretch long thin fibres from the VZ to the outer surface. Immature neurons
  wrap around these and crawl up with amoeba-like motion, driven by adhesion
  molecules like astrotactin.
- **Inside-out cortical layering.** The cortex is built inside out in six
  layers. The first wave forms the deepest layer (VI). The next climbs past VI
  to form V, the next past both to form IV, and so on to layer I. The youngest
  neurons always sit outermost.

### Stage 3. Axon guidance, synapse formation and circuit wiring (weeks 20–35)
*Biochemical navigation and network formation*

- **Growth cone navigation.** Once a neuron arrives, it grows a sensitive tip,
  the growth cone, with finger-like filopodia that sniff out chemical trails in
  the extracellular matrix.
- **Attraction and repulsion.** Four families of guidance molecules:
  - **Netrins and semaphorins**: soluble, diffuse through tissue, attract or
    repel the growth cone.
  - **Ephrins and CAMs** (cell adhesion molecules): bound to membranes, need
    direct contact, guide the axon over or around specific cell surfaces.
- **The synaptic handshake.** When the growth cone hits its exact target, the
  filopodia retract and the membrane becomes a presynaptic terminal. It
  releases neurexin, which binds neuroligin on the receiving cell, locking the
  two together into a working synapse.
- **Trophic factor competition.** The brain makes about 50% more neurons than
  survive into childhood. Arriving axons compete for a limited supply of
  neurotrophins (NGF, nerve growth factor; BDNF, brain-derived neurotrophic
  factor). Those that get enough survive; the rest die by apoptosis
  (programmed cell death), pruning the excess wiring.

### Stage 4. Folding, myelination and functional maturation (week 26 to term)
*Mechanical folding and electrical optimisation*

- **Mechanical tension and folding.** Around week 26 the smooth brain
  (lissencephalic) becomes folded (gyrencephalic). As neurons grow and
  connect, axons pulling between regions make the cortex buckle into ridges
  (gyri) and valleys (sulci). These organise separate functional maps and pack
  a huge surface into the skull.
- **Oligodendrocytes and myelin.** Oligodendrocytes multiply and wrap major
  axon tracts in layer after layer of fatty myelin.
- **The wave of myelination**, in strict order:
  1. Peripheral and brainstem tracts first: heart rate, swallowing, breathing,
     by week 24.
  2. Sensory pathways next: visual and auditory, so the late fetus processes
     sounds and flashes of light.
  3. Motor and cortical tracts: motor cortex down to the spinal cord begins
     next; higher regions like the prefrontal cortex keep myelinating into
     early adulthood.
- **Thalamocortical loops.** By weeks 26–28, pathways connect the thalamus
  (the sensory routing station) directly to the cortex. For the first time,
  body input is not only handled as brainstem reflexes but reaches the higher
  cortex. This is the framework for early awareness, pain perception and
  organised sleep–wake states.

---

## Part 3. The life story ("The Biological Blueprint: How the Mind Creates Reality")

### Phase 1. The blueprint and the separation (the womb)
- A flat disc of cells curls into a tube. Stem cells make 250,000 neurons a
  minute, to about 86 billion. Neurons crawl along glial ladders and build the
  layers inside out.
- **The senses switch on in a strict protective order**, to prevent overload:
  1. **Touch (the somatic baseline)**, at 8 weeks. The embryo's first act is a
     primitive flinch. This reflex loop is the origin of self-awareness: it
     sets up the binary loop "there is a Me, and there is a Not-Me" and maps
     the body's boundaries onto the parietal lobe.
  2. **Chemical senses**: taste and smell, the chemistry of the outside world.
  3. **Hearing**: the steady double-thump of the mother's heartbeat. It is so
     deeply wired that after birth, skin-to-skin contact instantly calms the
     limbic system through structural familiarity.
  4. **Vision**: kept completely dark until birth, protecting the visual
     cortex from premature flooding.

### Phase 2. The construction site (infancy to youth)
- Born with a pile of unlinked "cellular bricks"; the brain builds itself
  through real experience.
- **The sucking reflex.** Months of practising touch-and-open with amniotic
  fluid hardwire a sensory-motor highway between the parietal lobe (detecting
  touch) and the prefrontal cortex (coordinating the action). At birth the
  infant nurses automatically.
- **Serve and return.** Back-and-forth with a caregiver sprouts axons and
  dendrites, forging millions of synapses every second.
- **Pouring the concrete.** Repetition cements paths; heavily used tracks are
  myelinated, turning dirt roads into fibre-optic cables.
- **Sculpting (pruning).** By age 3 the brain has twice an adult's
  connections. Synaptic pruning removes unused ones, leaving a brain adapted
  to its own environment.

### Phase 3. The birth of logic and neural topography
- A child's brain starts as a pattern-seeking machine, not with abstract logic.
- **Action and consequence.** Repetition (dropping a spoon 100 times) links
  sensory data to outcomes, building physical "if/then" rules.
- **Retinotopy.** The brain lays out cells in patterns that mimic the input:
  when the retina sees a circle, visual cortex neurons fire in the shape of a
  circle.
- **Executive control.** The frontoparietal network (FPN), linking sensing
  (parietal) to planning (prefrontal), matures. The prefrontal cortex grows
  inhibitory pathways that can suppress the limbic system's emotional alarms,
  moving the child from pure emotion to rational logic.

### Phase 4. The lifelong shifting of the tides (adulthood to ageing)

| Age | Milestone |
|---|---|
| 10–14 | Maximum physical size: total brain volume peaks early |
| Early 20s | Raw processing speed peaks at 18–20; working memory around 25 |
| ~32 | Maximum pathway integration, structural stability, emotional regulation |
| 30–40 | Maximum wiring efficiency: myelin peaks |
| 38–45 | Maximum system integration: limbic and prefrontal networks in sync |
| 40s–60s | Leadership peak: mature prefrontal–limbic links manage stress, resist bias, use crystallised intelligence (wisdom) |
| Early 70s | Accelerated decay: shrinkage more than doubles, to 0.4%+ a year; myelin degrades; long-range networks like the FPN slow |

- **The muscle effect.** Regions grow or shrink with use (London taxi
  drivers' larger spatial hippocampus).
- Neuroplasticity never switches off; challenge and health cushion decline.

### The grand conclusion: "eating" the world
There is no line between abstract thought and biology. "Chewing on food"
breaks things down to build the body; "chewing on an idea" is the FPN breaking
down data and weaving it into wiring through plasticity. From a fetus touching
its lips, to a toddler telling cat from dog, to an adult in a crisis, learning
is the brain eating its environment to sculpt its own shape.

---

## Part 4. Frontal lobe

### Grooves
- **Central sulcus.** Boundary between thinking/moving (frontal) and feeling
  (parietal). **Idea:** a deeper central sulcus can give extraordinary
  dexterity, faster motor learning and hyper-acute touch, helping overall
  sensory understanding.
- **Precentral sulcus.** Border between the primary movement centre and the
  high-level planning and decision centres.
  - **Idea:** enhance the premotor cortex and supplementary motor area to push
    for longer, more complex chains of actions, once the baby is given a
    library and an understanding of physical bounds.
- **Inferior frontal sulcus/groove.** Cognitive flexibility, adaptation and
  rule switching, with working memory for logic and words. Keeps track of
  goals, numbers and abstract concepts in problem solving. Its back end runs
  into Broca's area (retrieving the correct word from memory during speech).
- **Superior frontal sulcus.** Spatial working memory and top-down attention
  control; supports the dorsal attention network and guides the frontal eye
  fields (FEF).

### Gyri and areas
- **Superior frontal gyrus (SFG), anteromedial part.** The default mode
  network (DMN): self-awareness, introspection, navigating complex social
  dynamics. Also the route to common sense.
- **Dorsolateral SFG.** Holds and manipulates complex concepts, numbers and
  rules; suppresses distraction for focus.
- **Middle frontal gyrus.** Fluid reasoning, devising novel strategies; works
  with the limbic system to understand threat and regulate emotion. Builds the
  attention networks: the DAN (dorsal attention network, top-down) and the VAN
  (ventral attention network, stimulus-driven); the middle frontal gyrus is
  where the two meet.
- **Inferior frontal gyrus (IFG) / Broca's area.** The core of Broca's area:
  turns abstract language and vocabulary into complex sequences. Pars
  opercularis generates the motor plans for speech. Broca's area corrects
  speech and builds articulate sentences. Pars orbitalis and pars triangularis
  are also listed as important.
- **Dorsolateral prefrontal cortex (DLPFC).** Manipulates working memory;
  strategy under pressure. The CEO of the brain.
  **Hartley goes here.** It reasons by elimination (Hartley, "Transmission of
  Information", 1928): hold the hypotheses as a set, rule out each one the
  evidence contradicts, and what it still doesn't know is log2 of what is
  left. Nothing is weighed or made likelier; every possibility counts the
  same. From the earlier chats: this is the left hemisphere's way (exact,
  eliminating); the right is associative (Shannon); the corpus callosum
  carries between them.
- **Premotor cortex.** Calculates the spatial orientation of objects.
- **Gyrus rectus.** Coordinates with the orbitofrontal cortex (OFC).
- **Orbitofrontal cortex.** Works with the amygdala in its calculations.
- **Anterior cingulate cortex (ACC).** Error-related negativity (the "that was
  a mistake" signal); chooses between conflicting options. The part that
  analyses mistakes.
- **Motor coordination.** Pyramidal neurons; Betz cells from the precentral
  gyrus to the spinal cord (see Part 1: no spinal cord, but a motor mechanism
  for touch).

### The author's hypotheses for the frontal lobe
1. **Broca + FEF hyper-direct.** "If Broca's area and the frontal eye fields
   were structurally deeper and had hyper-direct, heavily myelinated
   connections leading straight into the central sulcus." Goal: stronger
   interaction and clarity of novel ideas, paired with eye speed.
2. **Deeper grooves linking the planning hub.** What if the brain had deeper
   grooves connecting the superior frontal sulcus (spatial working memory,
   top-down attention guiding the FEF) and the inferior frontal groove
   (cognitive flexibility with working memory)? This would give a sharper
   understanding of the environment, working memory for logic and words,
   tracking of goals, numbers and abstract concepts, and better FEF guidance
   for vision.
3. **The DMN as a path to common sense** for how the newborn understands.
4. **Deep-connect the scattered planners.** Superior frontal sulcus, inferior
   frontal groove, dorsolateral SFG and the DLPFC are not deeply connected in
   a normal brain but scattered. Connect them deeply.

---

## Part 5. Parietal lobe

### Grooves
Three spatial boundaries: the **postcentral sulcus**, the **intraparietal
sulcus** and the **parieto-occipital sulcus** (relates to what you see; maps
boundaries). Focus is on the first two.

### Postcentral sulcus
On the lateral surface; separates the postcentral gyrus (in front) from the
superior and inferior parietal lobules (behind).

1. **Somatosensory boundary and spatial organisation.** Its anterior wall is
   the back limit of the somatosensory homunculus: the container for the
   body's map, with zones for touch from hands, face, torso and limbs. It
   collects body data; it does not understand it.
2. **Proprioceptive and nociceptive filtering.** A gatekeeper between raw
   sensation and high-order spatial awareness; isolates proprioceptive
   signals. **Idea:** proprioception enables self-understanding of the body;
   adapt it so that instead of being only about the body, it is about
   cognition too.
3. **Higher-order tactile and spatial processing.** The postcentral gyrus
   handles raw touch; the sulcus moves into secondary somatosensory
   processing, combining texture, roughness, edge direction and surface into
   one perception, to understand the configuration of objects.
4. **Stereognosis.** Its posterior rim supports identifying an object by touch
   alone (eyes closed, object in hand). **Idea:** use this to understand
   ideas through touch; mould ideas into things it can touch.
5. **Sensorimotor integration and feedback.** Being next to the motor cortex,
   it runs loops that feed real-time touch and position data to the motor
   side. **Idea:** build on this.

### Intraparietal sulcus (IPS)
Splits the superior from the inferior parietal lobule.

- **Dorsal visual stream hub.** Turns visual input from the occipital lobe into
  spatial maps: where objects are. **Idea:** use it to map position, location
  and changes.
- **Saccade calculation.** A subregion computes saccades (rapid jumps of the
  eyes between targets) and prioritises what deserves visual focus.
  Prioritisation matters.
- **Visually guided reaching and grasping.** Clusters compute exact
  trajectories; tied to the dorsal stream.
- **Internal number line.** The primary substrate for numerical processing and
  calculation. Feeds the symbolic work in the frontal lobe.
  **Idea:** combine with the inferior frontal gyrus, the medial frontal gyrus,
  and the DLPFC (given its workload and manipulation).
- **Non-symbolic estimation.** Long before reading numbers, a child instantly
  estimates quantities. Core to common sense. **Idea:** pair with the DMN in
  the anteromedial SFG.
- **Symbolic math calculation.** Symbolism plays a crucial role here.
- **Working memory and multimodal integration.** A temporary buffer for visual
  information that sets how many items you can track. Integrate with the
  frontal working memory.
- **Perceptual salience mapping.** A filter that builds a salience map:
  filters, focuses and prioritises.

### Supramarginal gyrus (SMG)
1. **Phonological language and working memory**
   - **Phonological loop buffer** (dominant, usually left): holds the sound
     segments of a word for several seconds (repeating words back, holding a
     phone number).
   - **Grapheme-to-phoneme conversion:** decodes written letters into sounds;
     sounding out new words.
   - **Articulatory loop feedback:** with Broca's area, monitors the auditory
     and motor feedback of your own voice so spoken words match intended
     ones.
2. **Complex praxis and motor planning**
   - **Tool-use ideation (ideomotor praxis):** stores motor plans and movement
     schemas for tools and objects (pen, key, hammer): the finger sequence and
     wrist angles.
   - **Action execution tracking:** turns abstract intentions into real-time
     instructions; spatial-motor coordinates for gestures (waving, saluting).
3. **Somatosensory integration and spatial self-awareness**
   - **Proprioceptive egocentric mapping:** combines touch with balance and
     vision into a model of your own body; where limbs are relative to torso.
   - **Peripersonal space:** monitors the zone within arm's reach; registers
     objects entering it so you can duck, dodge or block.
4. **Right-hemisphere specialisations**
   - **Stimulus-driven attention shifting:** a node of the ventral attention
     network; a tripwire that interrupts your thoughts to reorient toward
     sudden relevant changes (a loud noise, movement at the edge of vision).
   - **Egocentric empathy regulation:** overcomes emotional egocentricity;
     suppresses your own state to understand someone else's conflicting state.

### Angular gyrus (AG)
- **Visual-to-linguistic transduction** (left): the primary translator for
  written language; turns word shapes from the occipital lobe into internal
  auditory codes (subtitles under a movie use occipital and parietal
  together).
- **Symbol conversion:** symbols, facial expressions and letters registered as
  something meaningful and coherent.
- **Grapheme-to-semantic mapping:** links written words straight to meaning;
  maps a word's visual form to its entry in the mental dictionary.
- **Graphomotor output:** writing and spelling; holds the visual-spatial
  template of letters and directs the motor cortex.
- **Calculation from verbal memory** (left): exact arithmetic from memorised
  facts (multiplication tables, addition). This is memorised, not logic.
- **Abstract symbol processing:** numbers as magnitude and value, not shapes;
  works with the IPS on multi-step sequences. **Idea:** connect this with the
  frontal trio (inferior frontal groove, dorsolateral SFG, superior frontal
  gyrus) for efficiency.
- **Spatial navigation and body processing** (right): egocentric and
  allocentric maps; where your body is relative to surroundings.
- **Stable visual alignment** (right): combines inner-ear balance with vision
  for equilibrium and gravity orientation.
- **Self-location and body ownership:** the conscious sense of occupying your
  body.
- **Default mode network and social cognition:** a DMN hub (daydreaming,
  introspection); theory of mind (ToM) for social interaction.

---

## Part 6. Temporal lobe
"The one that was lacking the most."

### Grooves
- **Lateral (Sylvian) fissure.** The deepest horizontal trench on the side of
  the brain; if the central sulcus is the vertical line, this is the
  horizontal canyon separating the temporal lobe from frontal and parietal.
  It protects the deep language and auditory centres. Centre of human
  language, high-speed auditory processing and internal self-regulation.
  It was unusual in Einstein's brain. **Idea:** study how Einstein's Sylvian
  fissure formed, for the brain's formation. Language is part of the baby's
  understanding of everything, so the fissure must be used. Around it:
  - the language highway
  - the parietal network
  - front bank: IFG / Broca's area (pars opercularis, motor plans for speech)
  - back bank: Wernicke's area, translating heard or read words into
    meaningful thoughts.
  **Idea:** use these for recognising patterns and interpreting symbols into
  coherent codes it can understand.
- **Superior temporal sulcus (STS).** Parallel to the Sylvian fissure.
  Social cognition and the social brain: biological motion (walking), eye
  gaze, face processing; the epicentre for reading other living beings
  (facial expressions, eyes). Assists theory of mind. **Idea:** connect it to
  the symbolic representation work above, and to the ACC (mistake analysis).
- **Inferior temporal sulcus.** The object cataloguer, part of the ventral
  stream. While the upper network tracks movement, this identifies what an
  object is; processes shapes. It isolates an object's configuration and
  understands it. This is one of the two "configuration of objects" parts;
  the other is the postcentral sulcus, which does the same by touch (Part 5).
  **Idea:** connect the two.
- **Rhinal sulcus.** The entorhinal hub, the spatial GPS: the main interface
  between cortex and hippocampus. Grid cells build a cognitive map for
  navigating space. Nearby cortex (perirhinal) gives the flash of "I've seen
  this before"; wire it to memory. **Idea:** the same mechanism lets the code
  navigate the internet and know where it is, self-sufficiently.
- **Collateral sulcus.** Scene and landscape decoder: its outer bank holds the
  parahippocampal place area (PPA), which responds to scenes, landscapes,
  buildings and rooms, the larger picture, not isolated objects. **Idea:**
  "looking at the larger picture" as a way of problem solving, linked to the
  middle frontal gyrus's fluid reasoning; and memory encoding.
- **Collateral and rhinal together** separate the deep ancient emotional
  structures from the outer neocortex. **Idea:** useful for emotional and
  ethical development.

### Gyri
1. **Heschl's gyri (primary auditory cortex).** *Tonotopic mapping and spatial
   acoustics.*
   - Gateway for sound inside the lateral fissure; first cortex to receive raw
     signals from the cochlea via the medial geniculate body of the thalamus.
   - Tonotopic, like a piano keyboard: one end high pitch, the other low.
   - Basic feature extraction: pitch, volume, timbre, duration.
   - Binaural localisation: microsecond differences between ears give
     direction.
2. **Superior temporal gyrus (STG).** *Secondary auditory processing and
   language comprehension.*
   - Complex acoustic synthesis: footsteps vs dog bark vs instrument vs voice.
   - Phonological processing: changing frequencies over time → phonemes
     ("B" vs "P").
   - Wernicke's area (posterior left STG): decodes the structure of spoken
     sentences.
   - Prosody (right STG): melody, rhythm, intonation; angry, sarcastic, happy,
     sad from tone alone.
3. **Middle temporal gyrus (MTG).** *Lexical storage and multimodal
   integration.*
   - The mental lexicon: vocabulary linked to definitions and grammar.
   - Semantic context retrieval: resolves ambiguity ("bank" as river or money).
   - Action and tool concepts: verbs, "hammer", "run".
   - Multimodal bridge between auditory (above) and visual (below) into one
     idea.
4. **Inferior temporal gyrus (ITG).** *Ventral stream visual identification.*
   - High-level form synthesis: lines, shadows, colours, textures → shapes;
     the peak of the "what" pathway.
   - Viewpoint-invariant recognition: a mug from any angle.
   - Perceptual constancy: the same object in shadow, sunlight or partly
     hidden.
5. **Fusiform gyrus.** *Expert visual recognition.*
   - Fusiform face area: faces holistically, not as parts; stranger vs family.
   - Visual word form area (left): letter shapes, strings, whole words.
   - Within-category expertise: adapts to car models, bird species, chess
     positions.
6. **Parahippocampal gyrus.** *Spatial navigation and contextual memory.*
   - PPA: layout of scenes and rooms.
   - Memory gateway: gathers input from across cortex into the hippocampus for
     long-term episodic memory.
   - Cognitive maps: where you are, routes, orienting in new places.
   - Contextual association: a smell or sound retrieves the place, time and
     situation.

### Areas
- **Primary auditory cortex (A1),** within Heschl's gyri. *Transduction and
  acoustic analysis.*
  - Terminal for raw auditory data via the auditory nerve and thalamus.
  - Tonotopic grid: anterior low (bass), posterior high (treble).
  - Intensity and temporal gating: amplitude and microsecond timing of onset,
    change and stop, fed to language and music centres.
- **Wernicke's area.** *Semantic decoding and syntactic assembly.*
  - Meaning extraction: phonemes → words; matches sounds to stored concepts.
  - Syntax parsing: "the dog chased the cat" vs "the cat chased the dog".
  - Speech generation blueprinting: "Before you speak, Wernicke's area
    constructs the conceptual …" (**Open**: cut off in the source; the rest
    never arrived).

### Insula (hidden fifth lobe)
Deep in the Sylvian fissure under the frontal, parietal and temporal folds.
The master centre of interoception: conscious awareness of the body's internal
state. **Idea:** pair it with coded optogenetics so the body can understand how
it is working; base self-regulation on it.

### Coded optogenetics
Optogenetics, the 2026 Nobel Prize in Physiology or Medicine (Deisseroth,
Hegemann and Nagel: light-gated ion channels, switching chosen neurons on and
off with light). **Idea:** a coded version of it, used to selectively grow
chosen parts of the brain. It is already used in the code to switch parts off
and see what each is worth, and to light any one memory and read it back.

Auditory processing is to be discussed further; the current focus is
cognition and touch.

---

## Part 7. Cerebellum

Tightly packed neurons running fast calculations for micro-adjustments of
movement. **Idea:** use it as storage; it plays a crucial role in data storage.

### Structure of a folium
Every folium shares the same three-layer matrix.
- **Molecular layer (outer):** unmyelinated parallel fibres, Purkinje
  dendrites, inhibitory stellate and basket cells fine-tuning local signals.
- **Purkinje cell layer (middle):** one sheet of large Purkinje cells, the
  main computing units; their axons are the sole (inhibitory) output of the
  cerebellar cortex.
- **Granular layer (inner):** billions of tiny granule cells; the receiver,
  taking sensory and motor input and firing it up to the molecular layer.

### Wiring
- **Mossy fibres:** raw sensory data from the spinal cord and motor plans from
  the cortex into the granular layer, split into millions of parallel streams.
- **Climbing fibres:** only from the inferior olivary nucleus; one fibre wraps
  one Purkinje cell like ivy and fires strong impulses as an error signal when
  a movement is wrong.

### Functions
- **Feedforward motor coordination:** does not start movements; computes
  force, speed and trajectory so muscles fire smoothly.
- **Microsecond timing:** its geometry acts as internal clocks between input
  and output (typing, playing an instrument, catching).
- **Long-term depression (LTD):** the seat of motor learning. A climbing-fibre
  error changes that Purkinje cell's sensitivity to future input, damping
  wrong movements and strengthening right ones.
- **Predictive schema tuning:** builds predictive models; compares what the
  brain intended with what the body did, and closes the gap in real time.
- **Vestibulocerebellar equilibrium:** the flocculonodular lobe uses the inner
  ear for posture, trunk stability and the vestibulo-ocular reflex (stable
  vision while walking).
- **Cognitive and affective filtering:** posterior folia map to cognitive and
  emotional cortex; they smooth transitions in thought, language sequencing
  and emotion, preventing overshoot just as with movement.

---

## Part 8. From the earlier chats (not in the PDF)

Recorded in the Ciall repo (`research/BEING.md`, `research/ONE_BRAIN.md`)
from the conversations that set the project. To be confirmed by the author.

- **The limbic loop:** the amygdala regulating how much gets through; the
  hippocampus binding pairs; a router choosing which part acts on which; the
  cerebellum predicting and correcting.
- **Two hemispheres and the corpus callosum:** exact and eliminating (left,
  Hartley) and associative (right, Shannon). Genome ideas: a thicker corpus
  callosum, and Einstein's parietal lobe.
- **Baby Ciall as genes and older sibling:** the newborn is built from baby
  Ciall's design but starts with no memories. From about month three, the
  older sibling teaches it.
- **Eidetic memory, read by "coded optogenetics":** the hippocampus keeps
  every moment exactly; any moment can be lit and read back without the
  newborn knowing. (This is the light-a-memory feature on the page.)
- **Claims held as hypotheses, not facts:** that the first flinch is the
  origin of self-awareness, and that brain integration explains why leaders
  tend to be 40 to 60.

## Settled

- **"DML"** is the DMN (default mode network), anteromedial superior frontal
  gyrus.
- **"BAN"** is the VAN (ventral attention network).
- **Hartley** belongs to the DLPFC, the CEO of the brain (Part 4).
- **The three brains** are the grown brain, the life brain and the mind, and
  how they become one, region by region (Part 1).
- **Optogenetics:** a coded version of the 2026 Nobel work, to selectively
  grow chosen parts of the brain (Part 6).
- **The configuration of objects:** the inferior temporal sulcus, which
  isolates an object's configuration, together with the postcentral sulcus by
  touch.
- **"Dorsolateral SRG"** is the dorsolateral SFG; **"the marginal one"** is the
  supramarginal gyrus; **"Vernix area"** is Wernicke's area.

## Open questions

1. **Wernicke's "speech generation blueprinting"** is cut off mid-sentence
   ("constructs the conceptual …"); the rest never arrived.
2. **Which life stage switches on each idea**, and each big idea as a test
   that could fail: still to be discussed (noted as open in the earlier chats
   too).
