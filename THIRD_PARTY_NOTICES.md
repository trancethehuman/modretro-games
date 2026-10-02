# Third-party notices

Original documentation, content specifications, and validation tooling use the root MIT licence. The native game project also contains MIT-licensed starter artwork and upstream project metadata. No third-party runtime, ROM, or geographic dataset is vendored. The original Toronto scene extension and procedural pixel artwork use the root MIT licence; `td_font.h` derives from the retained MIT Bench Mono starter glyphs.

## Native starter assets and metadata

- The room, player, fixed-width font, dialogue frame and cursor were supplied by the [ModRetro Chromatic plugin](https://github.com/OpenAI-Partners/ModRetro-Chromatic-Plugin-for-Codex), version 1.0.33. Their copyright and permission notice is retained in [ASSET_LICENSE](games/toronto-dispatch/project/ASSET_LICENSE), copyright 2026 Eric Provencher. Editable indexed-pixel sources and provenance are in [original-art](games/toronto-dispatch/project/original-art/) and [STARTER_PROVENANCE.json](games/toronto-dispatch/project/STARTER_PROVENANCE.json).
- Eleven font glyphs reuse the plugin's original Signal Lost title glyph definitions at commit `5929961b938260ad112b87fc57145b7b23c7a196`; remaining starter artwork was authored for its release. This is starter art, not Toronto landmark art.
- Native GB Studio project metadata and included documentation retain [project/LICENSE](games/toronto-dispatch/project/LICENSE), copyright 2019–2026 Chris Maltby. The root MIT licence does not replace either upstream notice.

## Development tools

- ModRetro Chromatic plugin, GB Studio, GBDK, PyBoy, Binjgb and other compiler/emulator dependencies retain their own licences. They are installed outside this repository. Exact tested versions are recorded in [BUILD.md](games/toronto-dispatch/docs/BUILD.md). Audit linked runtime and distribution notices before publishing ROM releases.

## Geographic references

- City of Toronto [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/) and [Neighbourhoods](https://open.toronto.ca/dataset/neighbourhoods/) are planned geography references. Their portal pages link the Open Government Licence – Toronto. No geometry has been imported yet. Record applicable attribution and source/version details before importing data.
- TTC route pages and maps are factual research references. No TTC artwork, logo, map image, or transit dataset is included. Check source terms before copying or distributing material.
- Landmark imagery must be original pixel art with documented references; do not copy photographs or commercial game assets.
- Official PDF maps are reference material only and are retained in ignored local research storage. In particular, the Biidaasige Park map restricts reproduction, distribution and alteration without City permission. No map images are committed; source links and original observations are recorded in the [research report](games/toronto-dispatch/docs/TORONTO_RESEARCH.md).
