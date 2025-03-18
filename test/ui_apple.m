/*
 * Tibia
 *
 * Copyright (C) 2025 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo
 */

#import <Cocoa/Cocoa.h>

void *ui_create(char has_parent, void *parent, int width, int height) {
	NSView *v = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
/* WRONG
	if (has_parent) {
		NSView *p = (NSView *)parent;
		[p addSubview:v positioned:NSWindowAbove relativeTo:nil];
	}
*/
	return (void *)v;
}
