
#include "AnnotatedCode.h"

#include <r_util.h>
#include <r_cons.h>

R_API RAnnotatedCode *r_annotated_code_new(char *code)
{
	RAnnotatedCode *r = R_NEW0 (RAnnotatedCode);
	if (!r) {
		return NULL;
	}
	r->code = code;
	r_vector_init (&r->annotations, sizeof(RCodeAnnotation), NULL, NULL);
	return r;
}

R_API void r_annotated_code_free(RAnnotatedCode *code)
{
	r_vector_clear (&code->annotations);
	r_free (code->code);
	r_free (code);
}

R_API void r_annotated_code_add_annotation(RAnnotatedCode *code, RCodeAnnotation *annotation)
{
	r_vector_push (&code->annotations, annotation);
}

R_API RPVector *r_annotated_code_annotations_in(RAnnotatedCode *code, size_t offset)
{
	RPVector *r = r_pvector_new (NULL);
	if (!r) {
		return NULL;
	}
	RCodeAnnotation *annotation;
	r_vector_foreach (&code->annotations, annotation) {
		if (offset >= annotation->start && offset < annotation->end) {
			r_pvector_push (r, annotation);
		}
	}
	return r;
}

R_API void r_annotated_code_print_json(RAnnotatedCode *code)
{
	PJ *pj = pj_new ();
	if (!pj) {
		return;
	}

	pj_o (pj);
	pj_ks (pj, "code", code->code);

	pj_k (pj, "annotations");
	pj_a (pj);
	RCodeAnnotation *annotation;
	r_vector_foreach (&code->annotations, annotation) {
		pj_o (pj);
		pj_kn (pj, "start", (ut64)annotation->start);
		pj_kn (pj, "end", (ut64)annotation->end);
		switch (annotation->type) {
		case R_CODE_ANNOTATION_TYPE_OFFSET:
			pj_ks (pj, "type", "offset");
			pj_kn (pj, "offset", annotation->offset.offset);
			break;
        case R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT:
            pj_ks (pj, "type", "syntax_highlight");
            switch (annotation->syntax_highlight.type) {
                case R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD:
                    pj_ks (pj, "syntax_highlight", "keyword"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_COMMENT:
                    pj_ks (pj, "syntax_highlight", "comment"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE:
                    pj_ks (pj, "syntax_highlight", "datatype"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME:
                    pj_ks (pj, "syntax_highlight", "function_name"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER:
                    pj_ks (pj, "syntax_highlight", "function_parameter"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE:
                    pj_ks (pj, "syntax_highlight", "local_variable"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE:
                    pj_ks (pj, "syntax_highlight", "constant_variable"); 
                    break;
                case R_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE:
                    pj_ks (pj, "syntax_highlight", "global_variable"); 
                    break;
            }
		}
		pj_end (pj);
	}
	pj_end (pj);

	pj_end (pj);
	r_cons_printf ("%s\n", pj_string (pj));
	pj_free (pj);
}

R_API void r_annotated_code_print_with_syntax_highlighting(RAnnotatedCode* code, RCore *core)
{
    if (code->annotations.len == 0) {
        r_cons_printf("%s\n", code->code);
        return;
    }


    size_t cur = 0;
    size_t len = strlen(code->code);

	RCodeAnnotation *annotation;
	r_vector_foreach (&code->annotations, annotation) {
        if (annotation->type != R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT)
            continue;

        // (1/3)
        // now we have a syntax highlighting annotation.
        // pick a suitable color for it.
#define PALETE(x) (core->cons && core->cons->context->pal.x)? core->cons->context->pal.x 
        const char* color = Color_RESET;
        switch (annotation->syntax_highlight.type) {
        case R_SYNTAX_HIGHLIGHT_TYPE_COMMENT:
            color = PALETE(comment): Color_WHITE;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD:
            color = PALETE(pop): Color_MAGENTA;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE:
            color = PALETE(func_var_type): Color_BLUE;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME:
            color = PALETE(fname): Color_RED;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE:
            color = PALETE(num): Color_YELLOW;
        default:
            break;
        }

        // (2/3)
        // the chunk before the syntax highlighting annotation should not be colored
        for (; cur < annotation->start && cur < len; cur++)
            r_cons_printf("%c", code->code[cur]);

        // (3/3)
        // everything in between start and end inclusive should be highlighted
        r_cons_printf("%s", color);
        for (; cur < annotation->end && cur < len; cur++)
            r_cons_printf("%c", code->code[cur]);
        r_cons_printf("%s", Color_RESET);
    }
    // the rest of the decompiled code should be printed
    // without any highlighting since we don't have any annotations left
    while (cur < len) {
        r_cons_printf("%c", code->code[cur]);
        cur++;
    }
}
