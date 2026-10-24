# DejaVu Sans Mono

Unmodified `DejaVuSansMono.ttf` from Ubuntu's installed fonts-dejavu-core
package. The included LICENSE contains the Bitstream Vera terms and DejaVu
public-domain modification notice. Upstream: https://dejavu-fonts.github.io/.

`tools/build_ui_font.py` bakes ASCII coverage into a small header for the
freestanding desktop. It requires Pillow only when regenerating the header;
normal OS builds use the checked-in result. No runtime font service is needed.
